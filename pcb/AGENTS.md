# pcb — PompyBoard Hardware Design

## OVERVIEW

KiCad PCB design for the Wonkle tablet's PompyBoard — 244×137.5mm, 2-layer FR4 with GND and 3.3V power pours. Houses STM32F429 MCU, 209 DRV5055A4 Hall sensors (11×19 grid), 19 CD74HC4067 analog muxes, USB-C connector, and TPS62152 buck converter.

## STRUCTURE

```
pcb/
├── pompyboard.kicad_pcb      # Main PCB layout (4.2MB)
├── pompyboard.kicad_sch      # Main schematic
├── pompyboard.kicad_pro      # KiCad project config
├── pompyboard.kicad_prl      # Project local settings
├── sensor.kicad_sch          # Sensor array schematic (79KB)
├── sensor_column.kicad_sch   # Column mux schematic (79KB)
├── README.md                 # JLCPCB manufacturing instructions
└── .gitignore
```

## WHERE TO LOOK

| Task | File | Notes |
|------|------|-------|
| MCU pin assignments | `pompyboard.kicad_sch` | STM32F429IGTx, PA11/PA12 for USB, PE3-PE6 for mux select |
| Sensor connections | `sensor.kicad_sch` | 209 DRV5055A4 with 1kΩ series resistors for crosstalk reduction |
| Mux wiring | `sensor_column.kicad_sch` | 19× CD74HC4067, shared select lines, COM to ADC pins |
| Power supply | `pompyboard.kicad_sch` | TPS62152 3.3V buck, USB-C VBUS input |
| Board outline | `pompyboard.kicad_pcb` | 244×137.5mm, 4 mounting holes |
| Manufacturing | `README.md` | JLCPCB steps, BOM/placement, ~$137 for 5 boards (2 assembled) |

## KEY DESIGN FACTS

- **MCU**: STM32F429IGTx (Cortex-M4F, 1MB Flash, 192KB RAM)
- **Sensors**: 209× DRV5055A4 ratiometric linear Hall effect (analog output, 0.5-4.5V range)
- **Muxes**: 19× CD74HC4067 16:1 analog mux — one per column, shared 4-bit select on PE3-PE6
- **ADC pins**: ADC2 on PA1-PA7 + PC1-PC5, ADC3 on PF3-PF10
- **USB**: GCT USB4085 USB-C connector on PA11 (DM) / PA12 (DP), USB 2.0 only
- **Power**: TPS62152 3.3V buck converter, 3.3V pour on bottom copper
- **Crosstalk**: 1kΩ series resistors on each sensor output, mux delay in firmware
- **Thickness**: README says 1.0mm, PCB metadata says 1.6mm — unresolved discrepancy

## MANUFACTURING (JLCPCB)

```bash
# 1. Install KiCad + Fabrication Toolkit plugin
# 2. Generate production files (plugin → default settings)
# 3. Upload pcb/production/pompyboard.zip to jlcpcb.com
# 4. Configure:
#    - Thickness: 1.0mm (per README)
#    - Surface: LeadFree HASL
#    - PCBA Qty: min 2 (of 5 boards)
#    - Board Cleaning: Yes
# 5. Upload pcb/production/bom.csv + positions.csv
# 6. Fix J2 (1×4 pin header) placement manually
# 7. Category: Research/Education/DIY → DIY Hobby Circuit Board
```

## ANTI-PATTERNS

- **NEVER trust PCB thickness from metadata alone** — README and layout disagree (1.0mm vs 1.6mm). Verify before ordering.
- **NEVER skip board cleaning for PCBA** — flux residue on Hall sensors affects readings.
- **NEVER assume VCP/CDC serial** — no UART-to-USB bridge on board. Use SWD (probe-rs) for all debug.
- **KiCad project only** — no Altium/Eagle import. Must use KiCad.
