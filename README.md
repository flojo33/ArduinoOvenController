# ArduinoOvenController
Arduino project for controlling a heat treating oven.

The oven is controlled by a relay that triggers the heating coil. A PT1000 temperature sensor is installed in the oven. The arduino is connected to a 2x16 digit display including a rotary encoder as input.

The Software has features for editing and saving multiple programs with individual steps described by duration and target temperature. For example a Program could look like the following:

| Step | Duration | Target temperature |
|------|----------|--------------------|
|1     |010m      |050.0c              |
|2     |020m      |070.0c              |
|3     |030m      |090.0c              |
|4     |120m      |110.0c              |
|5     |120m      |075.0c              |

There is also a manual mode that allows setting a target temperature at runtime. The oven will then hold this temperature until the user stops the program.

The duty cycle is controlled by a simple temperature offset. This offset defines at what temperature the coil is enabled/disabled in relation to the current temperature received from the PT1000 sensor.

Hardware used:
- Elegoo Uno R3: https://www.amazon.de/dp/B0BJKDQ1VY?psc=1&ref=ppx_yo2ov_dt_b_product_details
- Charing Cable: https://www.amazon.de/dp/B081YDN4X8?psc=1&ref=ppx_yo2ov_dt_b_product_details
- Relay: https://www.amazon.de/dp/B07BVXT1ZK?psc=1&ref=ppx_yo2ov_dt_b_product_details
- Rotary Encoder: https://www.amazon.de/dp/B07SV5HHM5?ref=ppx_yo2ov_dt_b_product_details&th=1
- Display: https://www.amazon.de/dp/B0B76Z83Y4?psc=1&ref=ppx_yo2ov_dt_b_product_details
- PT1000 Board: https://www.amazon.de/dp/B079FZHVH7?psc=1&ref=ppx_yo2ov_dt_b_product_details
- Breadboard + Cables: https://www.amazon.de/dp/B0BTX3HN6H?ref=ppx_yo2ov_dt_b_product_details&th=1
