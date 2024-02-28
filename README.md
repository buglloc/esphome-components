# `ESPHome` components

A collection of my ESPHome components. Tested on ESP Home v2024.2.1 and may be broken on older ones :)

To use this repository you should confugure it inside your yaml-configuration:
```yaml
external_components:
  - source: github://buglloc/esphome-components
    refresh: 10min
```

You can take a look at samples of usage of those components in [examples](examples) folder.

## [axs15231](components/axs15231) display (wip)

[AXS15231](docs/datasheet/AXS15231_Datasheet_V0.4_20221108.pdf) Display used (and tested) on [T-Display S3 Long](https://www.lilygo.cc/products/t-display-s3-long):
![axs15231_demo](docs/images/axs15231_demo.jpg)

Requirements:
  - ESP-IDF framework so far
  - [Quad SPI](https://github.com/esphome/esphome/pull/5925)

Minimal example:
```yaml
external_components:
  - source: github://buglloc/esphome-components
    components: [ axs15231 ]

spi:
  id: quad_spi
  clk_pin: 17
  data_pins:
    - 13
    - 18
    - 21
    - 14

display:
  - platform: axs15231
    dimensions:
      height: 640
      width: 180
    auto_clear_enabled: false
    cs_pin: 12
    reset_pin: 16
    backlight_pin: 1
    rotation: 0
    lambda: |-
      it.fill(Color::random_color());
```

See [full example](examples/axs15231/t-display-s3-long.yaml) in [examples](examples) folder.
