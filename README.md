# Real-Time Sensor Data Acquisition System

A high-performance embedded sensor data acquisition system designed for ESP32-class microcontrollers, implemented in standard C11 with cross-platform simulation support.

**Course Project**: CPE2206 Group Project 1 | Week 7 Final Integration  
**Language**: C11 (no dynamic allocation)  
**Platform**: Windows/Linux simulation → ESP32 target

---

## Features

- **5 Simulated Sensors**: Temperature, Humidity, Soil Moisture, Light Intensity, CO2
- **Fixed-Point Arithmetic**: Q8.8 format (2-byte values) for memory efficiency
- **Zero Dynamic Allocation**: All memory statically allocated, malloc-free
- **Real-Time Processing**:
  - O(1) moving average with sliding window
  - O(1) anomaly detection (threshold + spike detection)
  - O(n) linear regression trend analysis
  - Min-heap priority queue for alert handling
- **Circular Buffers**: Power-of-2 sized with bit-mask wraparound
- **Performance Metrics**: Latency tracking, throughput monitoring
- **Cross-Platform**: Windows (QueryPerformanceCounter) and Linux (clock_gettime)

---

## Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        main.c                                   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────────┐   │
│  │ sensors  │→ │ circular │→ │ processing │→│ serial       │   │
│  │ .c/.h    │  │ buffer   │  │ .c/.h      │  │ monitor.c/.h │   │
│  └──────────┘  │ .c/.h    │  │            │  └──────────────┘   │
│                └──────────┘  │ • Moving   │                     │
│  Sensor                       │   Avg      │   Alert Display    │
│  simulation                   │ • Anomaly  │   & Logging        │
│                               │   Detect   │                     │
│                               │ • Trend    │                     │
│                               │   (LR)     │                     │
│                               └────────────┘                     │
│  ┌─────────────────────────┐                                     │
│  │ priority_queue.c/.h     │← Event queuing by urgency          │
│  │ (binary min-heap)       │                                    │
│  └─────────────────────────┘                                     │
└─────────────────────────────────────────────────────────────────┘
```

---

## File Structure

| File | Description |
|------|-------------|
| `main.c` | Entry point, tick loop, system orchestration |
| `config.h` | All tunable parameters (thresholds, sizes, rates) |
| `sensors.c/h` | Sensor simulation with realistic noise models |
| `circular_buffer.c/h` | Ring buffer with bit-mask indexing |
| `processing.c/h` | Signal processing: moving avg, anomaly detection, linear regression |
| `priority_queue.c/h` | Min-heap priority queue + alert log |
| `serial_monitor.c/h` | Formatted console output (UART simulation) |

---

## Configuration (`config.h`)

Key parameters you can tune without touching logic:

```c
/* Buffer sizes (must be powers of 2) */
#define TEMP_BUF_SIZE        16
#define HUMIDITY_BUF_SIZE     8
#define SOIL_BUF_SIZE         8
#define LIGHT_BUF_SIZE        8
#define CO2_BUF_SIZE         16

/* Sampling rates (sample every N ticks) */
#define TEMP_SAMPLE_RATE      1
#define HUMIDITY_SAMPLE_RATE  1
#define SOIL_SAMPLE_RATE      2   /* Soil changes slowly */
#define LIGHT_SAMPLE_RATE     1
#define CO2_SAMPLE_RATE       2

/* Anomaly thresholds (real-world units) */
#define TEMP_LOW_THRESH      10   /* °C */
#define TEMP_HIGH_THRESH     40   /* °C */
#define TEMP_SPIKE_THRESH     5   /* °C/tick */
```

---

## Building

### Windows (MinGW/MSVC)
```bash
gcc -O2 -Wall -o sensor_acq.exe main.c sensors.c circular_buffer.c processing.c priority_queue.c serial_monitor.c
```

### Linux/Mac
```bash
gcc -O2 -Wall -o sensor_acq main.c sensors.c circular_buffer.c processing.c priority_queue.c serial_monitor.c -lm
```

### ESP32 (Arduino IDE / PlatformIO)
Replace `simulate_sensor()` body with actual ADC reads:
```c
fp16_t simulate_sensor(uint8_t sensor_id, uint32_t tick) {
    return float_to_fp(adc_read_sensor(id));  // Replace with HAL call
}
```

---

## Running

```bash
./sensor_acq.exe    # Windows
./sensor_acq        # Linux/Mac
```

**Simulation Settings** (`config.h`):
- `SIM_TOTAL_TICKS = 0` → Run forever (Ctrl+C to stop)
- `SIM_TICK_DELAY_MS = 500` → 500ms between ticks

---

## Sample Output

```
╔══════════════════════════════════════════════════════════════════╗
║     REAL-TIME SENSOR DATA ACQUISITION SYSTEM                     ║
║     CPE2206 | Fixed-Point (Q8.8) | Zero malloc                   ║
╚══════════════════════════════════════════════════════════════════╝

Memory Report:
  Total static RAM: ~384 bytes (configurable)

[TICK 001] ─────────────────────────────────────────────────────────
Sensor        │ Raw   │ Avg   │ Trend     │ Status
──────────────┼───────┼───────┼───────────┼────────────────
Temperature   │ 24.50 │ 24.50 │ rising    │ OK
Humidity      │ 62.00 │ 62.00 │ stable    │ OK
Soil Moisture │ 45.00 │ 45.00 │ ─         │ OK
Light         │ 780.0 │ 780.0 │ stable    │ OK
CO2           │ 410.0 │ 410.0 │ ─         │ OK

[PERF] Samples: 4 | Alerts: 0 | Latency: 42.3 µs | Throughput: 94850 SPS
```

---

## Algorithm Complexities

| Operation | Complexity | Implementation |
|-----------|------------|----------------|
| Circular buffer write | O(1) | Bit-mask indexing |
| Moving average update | O(1) | Running sum, ring buffer |
| Anomaly detection | O(1) | Threshold comparison |
| Trend (linear regression) | O(n) | Least-squares on buffer |
| Priority queue push/pop | O(log n) | Binary min-heap |

---

## Data Types

- **`fp16_t`**: Q8.8 fixed-point (`int16_t`)
  - Range: -128.00 to +127.99
  - Resolution: ~0.004 units
  - Conversion: `float_to_fp()`, `fp_to_float()`

---

## Portability Notes

The code uses conditional compilation for platform-specific timing:

```c
#ifdef _WIN32
  #include <windows.h>
  #define sleep_ms(ms) Sleep(ms)
#else
  #include <unistd.h>
  #define sleep_ms(ms) usleep((ms)*1000)
#endif
```

---

## License

Academic project for CPE2206 coursework.
