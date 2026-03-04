import pygame
import subprocess

ELF_PATH = "../firmware/target/thumbv7em-none-eabihf/debug/firmware"
CHIP = "STM32F429IGTx"

ROWS = 11
COLS = 19
CELL_SIZE = 55
SCALE_WIDTH = 60
MAX_ADC = 4095
#MAX_ADC = 1023 



def parse_data(data, rows=ROWS, cols=COLS):
    values = [int(data[i:i+3], 16) for i in range(0, len(data), 3)]
    return [values[r * cols:(r + 1) * cols] for r in range(rows)]


def value_to_color(v):
    if v <= 0:
        return (0, 0, 0)

    t = v / MAX_ADC  # 0..1

    if t < 0.5:
        # czarny -> zielony
        g = int(t * 2 * 255)
        return (0, g, 0)
    else:
        # zielony -> czerwony
        r = int((t - 0.5) * 2 * 255)
        g = int((1 - (t - 0.5) * 2) * 255)
        return (r, g, 0)


def draw_grid(screen, grid, font):
    for r in range(ROWS):
        for c in range(COLS):
            value = grid[r][c]
            color = value_to_color(value)

            rect = pygame.Rect(
                c * CELL_SIZE,
                r * CELL_SIZE,
                CELL_SIZE,
                CELL_SIZE,
            )

            pygame.draw.rect(screen, color, rect)
            pygame.draw.rect(screen, (40, 40, 40), rect, 1)

            text = font.render(str(value), True, (255, 255, 255))
            text_rect = text.get_rect(center=rect.center)
            screen.blit(text, text_rect)


def draw_scale(screen, font):
    height = ROWS * CELL_SIZE
    x_start = COLS * CELL_SIZE + 10

    for y in range(height):
        t = 1 - (y / height)
        v = int(t * MAX_ADC)
        color = value_to_color(v)
        pygame.draw.line(
            screen,
            color,
            (x_start, y),
            (x_start + 30, y),
        )

    max_text = font.render(str(MAX_ADC), True, (255, 255, 255))
    min_text = font.render("0", True, (255, 255, 255))

    screen.blit(max_text, (x_start, 5))
    screen.blit(min_text, (x_start, height - 20))


def listen():
    pygame.init()
    width = COLS * CELL_SIZE + SCALE_WIDTH
    height = ROWS * CELL_SIZE
    screen = pygame.display.set_mode((width, height))
    pygame.display.set_caption("STM32 Sensor Grid")

    font = pygame.font.SysFont("consolas", 16)

    cmd = ["probe-rs", "run", "--chip", CHIP, ELF_PATH]

    process = subprocess.Popen(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1
    )

    grid = [[0]*COLS for _ in range(ROWS)]

    try:
        while True:
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    process.terminate()
                    pygame.quit()
                    return

            line = process.stdout.readline()
            if not line:
                continue

            line = line.strip()

            if 'S' in line and 'E' in line and len(line) == 643:
                data = line[14:][1:-1]
                grid = parse_data(data)
                print(data)

            screen.fill((0, 0, 0))
            draw_grid(screen, grid, font)
            draw_scale(screen, font)
            pygame.display.flip()

    except KeyboardInterrupt:
        process.terminate()
        pygame.quit()


if __name__ == "__main__":
    listen()
