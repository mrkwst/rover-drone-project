## Description
<!-- What does this PR do? -->

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Hardware documentation update
- [ ] Firmware improvement
- [ ] Ground station enhancement
- [ ] Refactoring / cleanup
- [ ] Safety improvement

## Related Issues
<!-- Link to issues: Fixes #123, Related to #456 -->

## Testing Done
- [ ] Firmware builds (`pio run`)
- [ ] Firmware flashes to ESP32
- [ ] Serial monitor shows expected boot sequence
- [ ] WiFi AP starts (Rover_AP)
- [ ] WebSocket connects from ground station
- [ ] Manual drive (WASD) works
- [ ] Emergency stop works (button + WS command)
- [ ] Sensors report values (US, MPU6050, line, battery)
- [ ] Python syntax check passes (`python -m py_compile`)
- [ ] Documentation updated (if applicable)

## Hardware Tested
- [ ] On breadboard
- [ ] On chassis (mounted)
- [ ] With battery power
- [ ] With USB power only

## Safety Checklist
- [ ] No changes to motor control safety logic without review
- [ ] Emergency stop still works (button + timeout + WS disconnect)
- [ ] Battery critical voltage triggers emergency stop
- [ ] Command timeout (500ms) triggers emergency stop

## Screenshots / Logs
<!-- Add serial monitor output, oscilloscope captures, photos -->

## Checklist
- [ ] Code follows project style
- [ ] Self-review completed
- [ ] Comments added for complex logic
- [ ] Documentation updated (pinout, wiring, README)
- [ ] No commented-out code left behind
- [ ] Conventional commit messages used