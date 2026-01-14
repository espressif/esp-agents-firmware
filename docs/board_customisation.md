# Board Customisation

If you want to run the agent on your own board, you can follow these steps to add support for your board:

1. **Create a board directory** under `examples/common/boards/`:
   - Your custom board needs to be a sub-directory of `examples/common/boards/`.

   ```bash
   mkdir -p examples/common/boards/my_custom_board
   ```

2. **Add board configuration files**:
   - `board_defs.h` - Define board-specific macros (LED pins, touch channels, display settings, etc.). Check the other boards for reference.
   - `sdkconfig.defaults` - This overrides the default ESP-IDF menuconfig options when your board is selected.
   - If your board is already available in [ESP Board Manager](https://github.com/espressif/esp-gmf/tree/main/packages/esp_board_manager/boards), just add the `.use_from_esp_board_manager` file in your board directory. and skip the next step.

3. **For boards not available in ESP Board Manager**, also add:
   - `board_devices.yaml` - Device configuration
   - `board_peripherals.yaml` - Peripheral configuration
   - Note: This step is only required if your board is a custom one and is not available in ESP Board Manager.
   - In this case, do not add the `.use_from_esp_board_manager` file in your board directory.

4. **Use your custom board**:

   ```bash
   idf.py select-board --board my_custom_board
   ```
