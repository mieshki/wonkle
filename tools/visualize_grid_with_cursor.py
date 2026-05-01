#!/usr/bin/env python3
"""
Wonkle Sensor Grid Visualizer — Weighted Centroid Only.

Reads the sensor grid from device RAM via pyOCD and renders
a real-time heatmap with weighted centroid cursor position.

Usage:
    python visualize_grid_with_cursor.py <elf_file>

Controls:
    S - Toggle show/hide cursor
    H - Toggle show/hide heatmap
    T - Toggle sensor value text overlay
    +/- - Adjust detection threshold
    ESC / Q - Quit
"""

import sys
import struct
import time
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
DEFAULT_THRESHOLD = 2200


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

    show_cursor = True
    show_heatmap = True
    show_text = False
    threshold = DEFAULT_THRESHOLD

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

            cursor_pos = estimate_centroid(grid, threshold)

            if show_cursor and cursor_pos is not None:
                draw_cursor(screen, cursor_pos, cell_w, cell_h, color=(255, 50, 50))

            fps_surface = font.render(f"FPS: {fps:.1f}", True, (255, 255, 0))
            screen.blit(fps_surface, (4, 4))

            thresh_surface = font.render(
                f"Threshold: {threshold} (+/- to adjust)", True, (255, 128, 0)
            )
            screen.blit(thresh_surface, (4, 22))

            if cursor_pos is not None:
                coord_surface = font.render(
                    f"Cursor: X={cursor_pos[0]:.2f}  Y={cursor_pos[1]:.2f}",
                    True,
                    (255, 50, 50),
                )
            else:
                coord_surface = font.render("Cursor: None (no pen)", True, (255, 0, 0))
            screen.blit(coord_surface, (4, 40))

            help_surface = font.render(
                "[S]how  [H]eatmap  [T]ext  [+/-]Thresh  [ESC]Quit",
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
