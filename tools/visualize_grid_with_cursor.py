#!/usr/bin/env python3
"""
Wonkle Sensor Grid Visualizer.

Supports two modes:
  debug:     reads raw sensor grid from RAM, renders heatmap + centroid
  production: reads computed cursor position from RAM, renders simple cursor view

Usage:
    python visualize_grid_with_cursor.py <elf_file> [--mode {debug,production}]

Controls:
    S - Toggle show/hide cursor
    H - Toggle show/hide heatmap   (debug only)
    T - Toggle sensor value text overlay (debug only)
    +/- - Adjust detection threshold (debug only)
    ESC / Q - Quit
"""

import argparse
import struct
import subprocess
import sys
import time
from typing import Optional, Tuple

import numpy as np
from pyocd.core.helpers import ConnectHelper
import pygame

SENSOR_ROWS = 11
SENSOR_COLS = 19
#ADDR_GRID = 0x20000060
ADDR_GRID = 0x20000178


INIT_WIDTH = 1140
INIT_HEIGHT = 660

ADC_MIDPOINT = 2048
DEFAULT_THRESHOLD = 2200


def get_symbol_address(elf_path: str, symbol: str) -> int:
    result = subprocess.run(
        ["arm-none-eabi-nm", elf_path],
        capture_output=True, text=True, check=True
    )
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == symbol:
            return int(parts[0], 16)
    raise RuntimeError(f"Symbol {symbol} not found in {elf_path}")


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


def draw_cursor_on_canvas(
    screen: pygame.Surface,
    pos: Tuple[float, float],
    color: Tuple[int, int, int] = (255, 50, 50),
    radius: int = 10,
) -> None:
    width, height = screen.get_size()
    margin = 60
    canvas_w = width - 2 * margin
    canvas_h = height - 2 * margin

    x_px = margin + int(pos[0] * canvas_w / (SENSOR_COLS - 1))
    y_px = margin + int(pos[1] * canvas_h / (SENSOR_ROWS - 1))

    pygame.draw.rect(screen, (64, 64, 64), (margin, margin, canvas_w, canvas_h), 2)
    pygame.draw.circle(screen, color, (x_px, y_px), radius, 2)
    pygame.draw.line(screen, color, (x_px - radius - 6, y_px), (x_px + radius + 6, y_px), 2)
    pygame.draw.line(screen, color, (x_px, y_px - radius - 6), (x_px, y_px + radius + 6), 2)


def run_debug_mode(session, screen, font, font_small):
    show_cursor = True
    show_heatmap = True
    show_text = False
    threshold = DEFAULT_THRESHOLD

    running = True
    frame_count = 0
    fps = 0.0
    fps_timer = time.time()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.VIDEORESIZE:
                screen = pygame.display.set_mode((event.w, event.h), pygame.RESIZABLE)
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

        data = session.target.read_memory_block8(ADDR_GRID, SENSOR_ROWS * SENSOR_COLS * 2)
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


def run_production_mode(session, screen, font, elf_path):
    addr_x = get_symbol_address(elf_path, "g_cursor_x")
    addr_y = get_symbol_address(elf_path, "g_cursor_y")
    addr_valid = get_symbol_address(elf_path, "g_cursor_valid")

    show_cursor = True
    running = True
    frame_count = 0
    fps = 0.0
    fps_timer = time.time()

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False
            elif event.type == pygame.VIDEORESIZE:
                screen = pygame.display.set_mode((event.w, event.h), pygame.RESIZABLE)
            elif event.type == pygame.KEYDOWN:
                if event.key in (pygame.K_ESCAPE, pygame.K_q):
                    running = False
                elif event.key == pygame.K_s:
                    show_cursor = not show_cursor

        data_x = session.target.read_memory_block8(addr_x, 4)
        data_y = session.target.read_memory_block8(addr_y, 4)
        data_valid = session.target.read_memory_block8(addr_valid, 1)

        cursor_x = struct.unpack("<f", bytes(data_x))[0]
        cursor_y = struct.unpack("<f", bytes(data_y))[0]
        cursor_valid = data_valid[0] != 0

        screen.fill((0, 0, 0))

        if show_cursor and cursor_valid:
            draw_cursor_on_canvas(screen, (cursor_x, cursor_y))
            coord_surface = font.render(
                f"Cursor: X={cursor_x:.2f}  Y={cursor_y:.2f}",
                True,
                (255, 50, 50),
            )
        else:
            coord_surface = font.render("Cursor: None (no pen)", True, (255, 0, 0))
        screen.blit(coord_surface, (4, 4))

        fps_surface = font.render(f"FPS: {fps:.1f}", True, (255, 255, 0))
        screen.blit(fps_surface, (4, 22))

        help_surface = font.render(
            "[S]how  [ESC]Quit",
            True,
            (128, 128, 128),
        )
        screen.blit(help_surface, (4, screen.get_height() - 18))

        pygame.display.flip()

        frame_count += 1
        now = time.time()
        elapsed = now - fps_timer
        if elapsed >= 0.5:
            fps = frame_count / elapsed
            frame_count = 0
            fps_timer = now


def main():
    parser = argparse.ArgumentParser(description="Wonkle Sensor Grid Visualizer")
    parser.add_argument("elf_file", help="Path to firmware ELF file")
    parser.add_argument(
        "--mode",
        choices=["debug", "production"],
        default="debug",
        help="Visualizer mode: debug reads raw grid, production reads cursor position",
    )
    args = parser.parse_args()

    pygame.init()
    screen = pygame.display.set_mode((INIT_WIDTH, INIT_HEIGHT), pygame.RESIZABLE)
    pygame.display.set_caption(f"Wonkle Visualizer ({args.mode})")
    font = pygame.font.SysFont(None, 18)
    font_small = pygame.font.SysFont(None, 14)

    with ConnectHelper.session_with_chosen_probe(target="stm32f429igtx") as session:
        session.options["frequency"] = 10000000
        session.board.target.elf = args.elf_file
        session.target.reset_and_halt()
        session.target.resume()
        time.sleep(1)

        if args.mode == "debug":
            run_debug_mode(session, screen, font, font_small)
        else:
            run_production_mode(session, screen, font, args.elf_file)

    pygame.quit()


if __name__ == "__main__":
    main()
