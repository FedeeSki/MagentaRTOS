# MagentaRTOS

## Compile guide (workflow usato)

```bash
mkdir -p build
cd build
# Specifichiamo che stiamo usando il Pico 2 (RP2350)
cmake -DPICO_BOARD=pico2 ..
make -j$(sysctl -n hw.ncpu)
```