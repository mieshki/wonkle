use core::u16;

use stm32f4xx_hal::{
    adc::{Adc, config::SampleTime},
    gpio::{Analog, Output, Pin},
    pac::{ADC2, ADC3},
};
use rtt_target::rprint;

pub static mut DATA: [u16; ROW_LEN * COL_LEN] = [0; ROW_LEN * COL_LEN];

const MAX: f32 = 10_000.;
const SAMPLE_TIME: SampleTime = SampleTime::Cycles_480;
const THRESHOLD: u16 = 2300;
const ROW_LEN: usize = 11;
const COL_LEN: usize = 19;
const DIAGNOSTIC_MODE: bool = true;

const MUX_SETTLING_DELAY: u32 = 2000; // ~24µs at 84MHz

const OVERSAMPLE_COUNT: u32 = 1;

const CHANNELS: [[bool; 4]; ROW_LEN] = [
    [false, true, false, true],
    [true, false, false, true],
    [false, false, false, true],
    [false, false, false, false],
    [true, false, false, false],
    [false, true, false, false],
    [true, true, false, false],
    [false, false, true, false],
    [true, false, true, false],
    [false, true, true, false],
    [true, true, true, false],
];

pub struct Sensor {
    adc2: Adc<ADC2>,
    adc3: Adc<ADC3>,

    s0: Pin<'E', 6, Output>,
    s1: Pin<'E', 5, Output>,
    s2: Pin<'E', 4, Output>,
    s3: Pin<'E', 3, Output>,

    in0: Pin<'F', 3, Analog>,
    in1: Pin<'F', 4, Analog>,
    in2: Pin<'F', 5, Analog>,
    in3: Pin<'F', 6, Analog>,
    in4: Pin<'F', 7, Analog>,
    in5: Pin<'F', 8, Analog>,
    in6: Pin<'F', 9, Analog>,
    in7: Pin<'F', 10, Analog>,
    in8: Pin<'C', 1, Analog>,
    in9: Pin<'C', 2, Analog>,
    in10: Pin<'C', 3, Analog>,
    in11: Pin<'A', 1, Analog>,
    in12: Pin<'A', 2, Analog>,
    in13: Pin<'A', 3, Analog>,
    in14: Pin<'A', 4, Analog>,
    in15: Pin<'A', 6, Analog>,
    in16: Pin<'A', 7, Analog>,
    in17: Pin<'C', 4, Analog>,
    in18: Pin<'C', 5, Analog>,
}

impl Sensor {
    pub fn new(
        adc2: Adc<ADC2>,
        adc3: Adc<ADC3>,
        s0: Pin<'E', 6, Output>,
        s1: Pin<'E', 5, Output>,
        s2: Pin<'E', 4, Output>,
        s3: Pin<'E', 3, Output>,
        in0: Pin<'F', 3, Analog>,
        in1: Pin<'F', 4, Analog>,
        in2: Pin<'F', 5, Analog>,
        in3: Pin<'F', 6, Analog>,
        in4: Pin<'F', 7, Analog>,
        in5: Pin<'F', 8, Analog>,
        in6: Pin<'F', 9, Analog>,
        in7: Pin<'F', 10, Analog>,
        in8: Pin<'C', 1, Analog>,
        in9: Pin<'C', 2, Analog>,
        in10: Pin<'C', 3, Analog>,
        in11: Pin<'A', 1, Analog>,
        in12: Pin<'A', 2, Analog>,
        in13: Pin<'A', 3, Analog>,
        in14: Pin<'A', 4, Analog>,
        in15: Pin<'A', 6, Analog>,
        in16: Pin<'A', 7, Analog>,
        in17: Pin<'C', 4, Analog>,
        in18: Pin<'C', 5, Analog>,
    ) -> Sensor {
        Sensor {
            adc2, adc3, s0, s1, s2, s3,
            in0, in1, in2, in3, in4, in5, in6, in7,
            in8, in9, in10, in11, in12, in13, in14,
            in15, in16, in17, in18,
        }
    }

    fn select_row(&mut self, i: usize) {
        if CHANNELS[i][0] {
            self.s0.set_high();
        } else {
            self.s0.set_low();
        }

        if CHANNELS[i][1] {
            self.s1.set_high();
        } else {
            self.s1.set_low();
        }

        if CHANNELS[i][2] {
            self.s2.set_high();
        } else {
            self.s2.set_low();
        }

        if CHANNELS[i][3] {
            self.s3.set_high();
        } else {
            self.s3.set_low();
        }
    }

    fn read_row(&mut self, i: usize) {
        self.select_row(i);
        
        cortex_m::asm::delay(MUX_SETTLING_DELAY);

        unsafe {
            if OVERSAMPLE_COUNT > 1 {
                let mut sum: u32 = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in0, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 0] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in1, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 1] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in2, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 2] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in3, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 3] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in4, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 4] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in5, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 5] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in6, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 6] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc3.convert(&self.in7, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 7] = (sum / OVERSAMPLE_COUNT) as u16;

                // ADC2 - pozostałe 11 kanałów
                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in8, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 8] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in9, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 9] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in10, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 10] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in11, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 11] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in12, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 12] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in13, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 13] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in14, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 14] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in15, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 15] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in16, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 16] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in17, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 17] = (sum / OVERSAMPLE_COUNT) as u16;

                sum = 0;
                for _ in 0..OVERSAMPLE_COUNT {
                    sum += self.adc2.convert(&self.in18, SAMPLE_TIME) as u32;
                }
                DATA[i * COL_LEN + 18] = (sum / OVERSAMPLE_COUNT) as u16;
            } else {
                // w/o oversampling
                DATA[i * COL_LEN + 0] = self.adc3.convert(&self.in0, SAMPLE_TIME);
                DATA[i * COL_LEN + 1] = self.adc3.convert(&self.in1, SAMPLE_TIME);
                DATA[i * COL_LEN + 2] = self.adc3.convert(&self.in2, SAMPLE_TIME);
                DATA[i * COL_LEN + 3] = self.adc3.convert(&self.in3, SAMPLE_TIME);
                DATA[i * COL_LEN + 4] = self.adc3.convert(&self.in4, SAMPLE_TIME);
                DATA[i * COL_LEN + 5] = self.adc3.convert(&self.in5, SAMPLE_TIME);
                DATA[i * COL_LEN + 6] = self.adc3.convert(&self.in6, SAMPLE_TIME);
                DATA[i * COL_LEN + 7] = self.adc3.convert(&self.in7, SAMPLE_TIME);
                DATA[i * COL_LEN + 8] = self.adc2.convert(&self.in8, SAMPLE_TIME);
                DATA[i * COL_LEN + 9] = self.adc2.convert(&self.in9, SAMPLE_TIME);
                DATA[i * COL_LEN + 10] = self.adc2.convert(&self.in10, SAMPLE_TIME);
                DATA[i * COL_LEN + 11] = self.adc2.convert(&self.in11, SAMPLE_TIME);
                DATA[i * COL_LEN + 12] = self.adc2.convert(&self.in12, SAMPLE_TIME);
                DATA[i * COL_LEN + 13] = self.adc2.convert(&self.in13, SAMPLE_TIME);
                DATA[i * COL_LEN + 14] = self.adc2.convert(&self.in14, SAMPLE_TIME);
                DATA[i * COL_LEN + 15] = self.adc2.convert(&self.in15, SAMPLE_TIME);
                DATA[i * COL_LEN + 16] = self.adc2.convert(&self.in16, SAMPLE_TIME);
                DATA[i * COL_LEN + 17] = self.adc2.convert(&self.in17, SAMPLE_TIME);
                DATA[i * COL_LEN + 18] = self.adc2.convert(&self.in18, SAMPLE_TIME);
            }
        }
    }

    // pub fn get_raw_data(&mut self) {
    //     unsafe {
    //         rprint!("S"); 
    //         
    //         for r in 0..ROW_LEN {
    //             self.read_row(r);
    //             
    //             for c in 0..COL_LEN {
    //                 rprint!("{:03X}", DATA[r * COL_LEN + c]);
    //             }
    //         }
    //         
    //         rprint!("E\n"); 
    //     }
    // }

    pub fn get_raw_data(&mut self) {
        unsafe {
            if DIAGNOSTIC_MODE {
                // Test: Odczytaj ten sam wiersz wielokrotnie
                rprint!("S");
                
                for _ in 0..ROW_LEN {
                    self.select_row(0); // Zawsze ten sam wiersz
                    cortex_m::asm::delay(MUX_SETTLING_DELAY * 10);
                    
                    for col in 0..COL_LEN {
                        let val = match col {
                            0 => self.adc3.convert(&self.in0, SAMPLE_TIME),
                            1 => self.adc3.convert(&self.in1, SAMPLE_TIME),
                            2 => self.adc3.convert(&self.in2, SAMPLE_TIME),
                            3 => self.adc3.convert(&self.in3, SAMPLE_TIME),
                            4 => self.adc3.convert(&self.in4, SAMPLE_TIME),
                            5 => self.adc3.convert(&self.in5, SAMPLE_TIME),
                            6 => self.adc3.convert(&self.in6, SAMPLE_TIME),
                            7 => self.adc3.convert(&self.in7, SAMPLE_TIME),
                            8 => self.adc2.convert(&self.in8, SAMPLE_TIME),
                            9 => self.adc2.convert(&self.in9, SAMPLE_TIME),
                            10 => self.adc2.convert(&self.in10, SAMPLE_TIME),
                            11 => self.adc2.convert(&self.in11, SAMPLE_TIME),
                            12 => self.adc2.convert(&self.in12, SAMPLE_TIME),
                            13 => self.adc2.convert(&self.in13, SAMPLE_TIME),
                            14 => self.adc2.convert(&self.in14, SAMPLE_TIME),
                            15 => self.adc2.convert(&self.in15, SAMPLE_TIME),
                            16 => self.adc2.convert(&self.in16, SAMPLE_TIME),
                            17 => self.adc2.convert(&self.in17, SAMPLE_TIME),
                            18 => self.adc2.convert(&self.in18, SAMPLE_TIME),
                            _ => 0,
                        };
                        rprint!("{:03X}", val);
                    }
                }
                
                rprint!("E\n");
            } else {
                rprint!("S");
                for row in 0..ROW_LEN {
                    self.read_row(row);
                    for col in 0..COL_LEN {
                        rprint!("{:03X}", DATA[row * COL_LEN + col]);
                    }
                }
                rprint!("E\n");
            }
        }
    }

}
