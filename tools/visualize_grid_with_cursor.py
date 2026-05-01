#!/usr/bin/env python3
"""
Enhanced Wonkle Sensor Grid Visualizer with SOTA Cursor Position Estimation.

Extends the original visualize_grid.py with multiple sub-pixel cursor detection
algorithms, interactive algorithm switching, and real-time coordinate display.

Usage:
    python visualize_grid_with_cursor.py <elf_file>

Controls:
    1 - Weighted Centroid (baseline, matches firmware)
    2 - Separable Parabolic Interpolation (sub-pixel, fast)
    3 - 2D Gaussian Peak Fit (sub-pixel, most accurate)
    4 - Iterative Windowed Centroid (reduces edge bias)
    5 - Local Windowed Centroid (eliminates distant-sensor pull)
    A - Toggle adaptive threshold (threshold = 50% of peak value)
    S - Toggle show/hide cursor
    H - Toggle show/hide heatmap
    T - Toggle sensor value text overlay
    +/- - Adjust fixed detection threshold
    ESC / Q - Quit
"""

import sys
import struct
import time
from enum import IntEnum
from typing import Optional, Tuple

import numpy as np
from pyocd.core.helpers import ConnectHelper
import pygame

SENSOR_ROWS = 11
SENSOR_COLS = 19
ADDR = 0x20000060

INIT_WIDTH = 1140
INIT_HEIGHT = 660

ADC_MIDPOINT = 2048
DEFAULT_THRESHOLD = 2300
SUBPIXEL_FACTOR = 100.0


class Algorithm(IntEnum):
    CENTROID = 1
    PARABOLIC = 2
    GAUSSIAN_FIT = 3
    ITERATIVE_WINDOW = 4
    LOCAL_CENTROID = 5


def hsv_to_rgb(h_degrees: float, saturation: float, value: float) -> Tuple[int, int, int]:
    h = h_degrees % 360
    chroma = value * saturation
    secondary = chroma * (1 - abs((h / 60) % 2 - 1))
    match_value = value - chroma
    if h < 60:
        r, g, b = chroma, secondary, 0.0
    elif h < 120:
        r, g, b = secondary, chroma, 0.0
    elif h < 180:
        r, g, b = 0.0, chroma, secondary
    elif h < 240:
        r, g, b = 0.0, secondary, chroma
    elif h < 300:
        r, g, b = secondary, 0.0, chroma
    else:
        r, g, b = chroma, 0.0, secondary
    return (
        int((r + match_value) * 255),
        int((g + match_value) * 255),
        int((b + match_value) * 255),
    )


def adc_to_heatmap_color(adc_value: int) -> Tuple[int, int, int]:
    if adc_value <= ADC_MIDPOINT:
        return hsv_to_rgb(240, 1.0, 1.0)
    t = (adc_value - ADC_MIDPOINT) / (4095 - ADC_MIDPOINT)
    hue = 240 * (1 - t)
    return hsv_to_rgb(hue, 1.0, 1.0)


def threshold_signal(val: int, threshold: int) -> int:
    return max(0, val - threshold)


def estimate_centroid(
    grid: np.ndarray, threshold: int
) -> Optional[Tuple[float, float]]:
    vals = np.maximum(grid.astype(np.float64) - threshold, 0.0)
    total = vals.sum()
    if total <= 0:
        return None

    rows = np.arange(SENSOR_ROWS, dtype=np.float64)
    cols = np.arange(SENSOR_COLS, dtype=np.float64)

    sum_x = (vals.sum(axis=0) * cols).sum()
    sum_y = (vals.sum(axis=1) * rows).sum()

    cx = sum_x / total
    cy = sum_y / total
    return (cx, cy)


def estimate_parabolic(
    grid: np.ndarray, threshold: int
) -> Optional[Tuple[float, float]]:
    vals = np.maximum(grid.astype(np.float64) - threshold, 0.0)
    if vals.max() <= 0:
        return None

    coarse = estimate_centroid(grid, threshold)
    if coarse is None:
        return None

    cy_int, cx_int = int(round(coarse[1])), int(round(coarse[0]))
    cy_int = np.clip(cy_int, 0, SENSOR_ROWS - 1)
    cx_int = np.clip(cx_int, 0, SENSOR_COLS - 1)

    row = vals[cy_int, :]
    cx = float(cx_int)
    if 0 < cx_int < SENSOR_COLS - 1:
        vm1 = row[cx_int - 1]
        v0 = row[cx_int]
        vp1 = row[cx_int + 1]
        denom = vm1 - 2.0 * v0 + vp1
        if abs(denom) > 1e-6:
            cx = cx_int + 0.5 * (vm1 - vp1) / denom

    col = vals[:, cx_int]
    cy = float(cy_int)
    if 0 < cy_int < SENSOR_ROWS - 1:
        vm1 = col[cy_int - 1]
        v0 = col[cy_int]
        vp1 = col[cy_int + 1]
        denom = vm1 - 2.0 * v0 + vp1
        if abs(denom) > 1e-6:
            cy = cy_int + 0.5 * (vm1 - vp1) / denom

    return (cx, cy)


def estimate_gaussian_fit(
    grid: np.ndarray, threshold: int, window: int = 3
) -> Optional[Tuple[float, float]]:
    vals = np.maximum(grid.astype(np.float64) - threshold, 0.0)
    if vals.max() <= 0:
        return None

    peak_y, peak_x = np.unravel_index(np.argmax(vals), vals.shape)

    half = window // 2
    y0 = max(0, peak_y - half)
    y1 = min(SENSOR_ROWS, peak_y + half + 1)
    x0 = max(0, peak_x - half)
    x1 = min(SENSOR_COLS, peak_x + half + 1)

    local = vals[y0:y1, x0:x1]
    mask = local > 0
    if mask.sum() < 6:
        return estimate_centroid(grid, threshold)

    ys, xs = np.where(mask)
    ys = ys + y0
    xs = xs + x0
    zs = local[mask]

    log_z = np.log(zs + 1e-6)

    A = np.column_stack((xs * xs, ys * ys, xs, ys, np.ones_like(xs)))

    try:
        coeff, *_ = np.linalg.lstsq(A, log_z, rcond=None)
    except np.linalg.LinAlgError:
        return estimate_centroid(grid, threshold)

    a, b, c, d, _ = coeff
    cx = -c / (2.0 * a) if abs(a) > 1e-6 and a < 0 else float(peak_x)
    cy = -d / (2.0 * b) if abs(b) > 1e-6 and b < 0 else float(peak_y)

    cx = max(-0.5, min(SENSOR_COLS - 0.5, cx))
    cy = max(-0.5, min(SENSOR_ROWS - 0.5, cy))

    return (cx, cy)


def estimate_iterative_window(
    grid: np.ndarray,
    threshold: int,
    iterations: int = 5,
    window_sigma: float = 1.5,
) -> Optional[Tuple[float, float]]:
    vals = np.maximum(grid.astype(np.float64) - threshold, 0.0)
    if vals.max() <= 0:
        return None

    coarse = estimate_centroid(grid, threshold)
    if coarse is None:
        return None

    cx, cy = coarse
    rows = np.arange(SENSOR_ROWS, dtype=np.float64)
    cols = np.arange(SENSOR_COLS, dtype=np.float64)
    col_grid, row_grid = np.meshgrid(cols, rows)

    for _ in range(iterations):
        wx = np.exp(-0.5 * ((col_grid - cx) / window_sigma) ** 2)
        wy = np.exp(-0.5 * ((row_grid - cy) / window_sigma) ** 2)
        weights = wx * wy

        weighted = vals * weights
        total = weighted.sum()
        if total <= 0:
            break

        cx = (weighted * col_grid).sum() / total
        cy = (weighted * row_grid).sum() / total

    return (cx, cy)


def estimate_local_centroid(
    grid: np.ndarray, threshold: int, window: int = 3
) -> Optional[Tuple[float, float]]:
    vals = np.maximum(grid.astype(np.float64) - threshold, 0.0)
    if vals.max() <= 0:
        return None

    peak_y, peak_x = np.unravel_index(np.argmax(vals), vals.shape)

    half = window // 2
    y0 = max(0, peak_y - half)
    y1 = min(SENSOR_ROWS, peak_y + half + 1)
    x0 = max(0, peak_x - half)
    x1 = min(SENSOR_COLS, peak_x + half + 1)

    local = vals[y0:y1, x0:x1]
    local_rows = np.arange(y0, y1, dtype=np.float64)
    local_cols = np.arange(x0, x1, dtype=np.float64)

    total = local.sum()
    if total <= 0:
        return None

    sum_x = (local.sum(axis=0) * local_cols).sum()
    sum_y = (local.sum(axis=1) * local_rows).sum()

    cx = sum_x / total
    cy = sum_y / total
    return (cx, cy)


ESTIMATORS = {
    Algorithm.CENTROID: estimate_centroid,
    Algorithm.PARABOLIC: estimate_parabolic,
    Algorithm.GAUSSIAN_FIT: estimate_gaussian_fit,
    Algorithm.ITERATIVE_WINDOW: estimate_iterative_window,
    Algorithm.LOCAL_CENTROID: estimate_local_centroid,
}

ALGORITHM_NAMES = {
    Algorithm.CENTROID: "Weighted Centroid",
    Algorithm.PARABOLIC: "Parabolic Interpolation",
    Algorithm.GAUSSIAN_FIT: "2D Gaussian Fit",
    Algorithm.ITERATIVE_WINDOW: "Iterative Windowed",
    Algorithm.LOCAL_CENTROID: "Local Windowed Centroid",
}


def draw_cursor(
    screen: pygame.Surface,
    pos: Tuple[float, float],
    cell_w: int,
    cell_h: int,
    color: Tuple[int, int, int] = (0, 255, 0),
    radius: int = 8,
) -> None:
    x_px = int(pos[0] * cell_w + cell_w / 2)
    y_px = int(pos[1] * cell_h + cell_h / 2)

    pygame.draw.circle(screen, color, (x_px, y_px), radius, 2)
    pygame.draw.line(
        screen, color, (x_px - radius - 4, y_px), (x_px + radius + 4, y_px), 2
    )
    pygame.draw.line(
        screen, color, (x_px, y_px - radius - 4), (x_px, y_px + radius + 4), 2
    )


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <elf_file>")
        sys.exit(1)

    elf_path = sys.argv[1]

    pygame.init()
    screen = pygame.display.set_mode((INIT_WIDTH, INIT_HEIGHT), pygame.RESIZABLE)
    pygame.display.set_caption("Wonkle Sensor Grid + Cursor (pyOCD)")
    font = pygame.font.SysFont(None, 18)
    font_small = pygame.font.SysFont(None, 14)

    current_algorithm = Algorithm.GAUSSIAN_FIT
    show_cursor = True
    show_heatmap = True
    show_text = False
    threshold = DEFAULT_THRESHOLD
    adaptive_threshold = False
    adaptive_fraction = 0.5

    with ConnectHelper.session_with_chosen_probe(target="stm32f429igtx") as session:
        session.options["frequency"] = 10000000
        session.board.target.elf = elf_path
        session.target.reset_and_halt()
        session.target.resume()
        time.sleep(1)

        running = True
        frame_count = 0
        fps = 0.0
        fps_timer = time.time()

        while running:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.VIDEORESIZE:
                    screen = pygame.display.set_mode(
                        (event.w, event.h), pygame.RESIZABLE
                    )
                elif event.type == pygame.KEYDOWN:
                    if event.key in (pygame.K_ESCAPE, pygame.K_q):
                        running = False
                    elif event.key == pygame.K_1:
                        current_algorithm = Algorithm.CENTROID
                    elif event.key == pygame.K_2:
                        current_algorithm = Algorithm.PARABOLIC
                    elif event.key == pygame.K_3:
                        current_algorithm = Algorithm.GAUSSIAN_FIT
                    elif event.key == pygame.K_4:
                        current_algorithm = Algorithm.ITERATIVE_WINDOW
                    elif event.key == pygame.K_5:
                        current_algorithm = Algorithm.LOCAL_CENTROID
                    elif event.key == pygame.K_a:
                        adaptive_threshold = not adaptive_threshold
                    elif event.key == pygame.K_s:
                        show_cursor = not show_cursor
                    elif event.key == pygame.K_h:
                        show_heatmap = not show_heatmap
                    elif event.key == pygame.K_t:
                        show_text = not show_text
                    elif event.key == pygame.K_EQUALS or event.key == pygame.K_PLUS:
                        threshold = min(4000, threshold + 50)
                    elif event.key == pygame.K_MINUS:
                        threshold = max(0, threshold - 50)

            data = session.target.read_memory_block8(
                ADDR, SENSOR_ROWS * SENSOR_COLS * 2
            )
            vals = struct.unpack(f"<{SENSOR_ROWS * SENSOR_COLS}H", bytes(data))
            grid = np.array(vals, dtype=np.uint16).reshape(SENSOR_ROWS, SENSOR_COLS)

            active_threshold = threshold
            if adaptive_threshold:
                peak = int(grid.max())
                active_threshold = max(0, int(peak * adaptive_fraction))

            width, height = screen.get_size()
            cell_w = width // SENSOR_COLS
            cell_h = height // SENSOR_ROWS

            screen.fill((0, 0, 0))

            if show_heatmap:
                for r in range(SENSOR_ROWS):
                    for c in range(SENSOR_COLS):
                        val = grid[r, c]
                        color = adc_to_heatmap_color(val)
                        rect = pygame.Rect(c * cell_w, r * cell_h, cell_w, cell_h)
                        pygame.draw.rect(screen, color, rect)

            if show_text:
                for r in range(SENSOR_ROWS):
                    for c in range(SENSOR_COLS):
                        text_surf = font_small.render(str(grid[r, c]), True, (255, 255, 255))
                        text_rect = text_surf.get_rect(
                            center=(c * cell_w + cell_w // 2, r * cell_h + cell_h // 2)
                        )
                        screen.blit(text_surf, text_rect)

            estimator = ESTIMATORS[current_algorithm]
            cursor_pos = estimator(grid, active_threshold)

            if show_cursor and cursor_pos is not None:
                draw_cursor(screen, cursor_pos, cell_w, cell_h, color=(255, 50, 50))

            fps_surface = font.render(f"FPS: {fps:.1f}", True, (255, 255, 0))
            screen.blit(fps_surface, (4, 4))

            algo_surface = font.render(
                f"Algo: {ALGORITHM_NAMES[current_algorithm]} (press 1-5)",
                True,
                (0, 255, 255),
            )
            screen.blit(algo_surface, (4, 22))

            if adaptive_threshold:
                thresh_surface = font.render(
                    f"Threshold: ADAPTIVE ({active_threshold}) [A]", True, (0, 255, 128)
                )
            else:
                thresh_surface = font.render(
                    f"Threshold: {threshold} (+/- to adjust) [A]", True, (255, 128, 0)
                )
            screen.blit(thresh_surface, (4, 40))

            if cursor_pos is not None:
                coord_surface = font.render(
                    f"Cursor: X={cursor_pos[0]:.2f}  Y={cursor_pos[1]:.2f}",
                    True,
                    (255, 50, 50),
                )
            else:
                coord_surface = font.render("Cursor: None (no pen)", True, (255, 0, 0))
            screen.blit(coord_surface, (4, 58))

            help_surface = font.render(
                "[1-5]Algo  [A]daptive  [S]how  [H]eatmap  [T]ext  [+/-]Thresh  [ESC]Quit",
                True,
                (128, 128, 128),
            )
            screen.blit(help_surface, (4, height - 18))

            pygame.display.flip()

            frame_count += 1
            now = time.time()
            elapsed = now - fps_timer
            if elapsed >= 0.5:
                fps = frame_count / elapsed
                frame_count = 0
                fps_timer = now

    pygame.quit()


if __name__ == "__main__":
    main()
