#!/usr/bin/env python3
"""
WebSocket Test Client voor ESP32 Rover
Run op Raspberry Pi (Ground Station)

Gebruik:
    python3 ws_test.py                    # Interactieve test
    python3 ws_test.py --auto             # Auto test sequence
    python3 ws_test.py --joystick         # Joystick control (vereist pygame)
"""

import asyncio
import json
import sys
import argparse
import time
import signal
from typing import Optional

try:
    import websockets
except ImportError:
    print("Install: pip install websockets")
    sys.exit(1)

# ==================== CONFIG ====================
ROVER_IP = "192.168.4.1"       # ESP32 AP IP (standaard)
ROVER_PORT = 80
WS_PATH = "/ws"
WS_URL = f"ws://{ROVER_IP}:{ROVER_PORT}{WS_PATH}"

# ==================== COMMAND HELPERS ====================
def make_command(throttle: int = 0, steer: int = 0, mode: int = 0, estop_clear: bool = False) -> str:
    """Maak JSON commando: {'T': throttle, 'S': steer, 'M': mode, 'E': estop_clear}
    M: 0=manual, 1=auto (PID), 2=clear emergency stop
    E: true to clear emergency stop (alternative to M=2)
    """
    return json.dumps({
        "T": max(-100, min(100, throttle)),
        "S": max(-100, min(100, steer)),
        "M": mode,
        "E": estop_clear
    })

def parse_telemetry(json_str: str) -> dict:
    """Parse telemetrie JSON van rover"""
    try:
        return json.loads(json_str)
    except json.JSONDecodeError:
        return {}

# ==================== TEST SEQUENCES ====================
async def test_basic_movement(ws):
    """Test basis bewegingen"""
    print("\n🧪 TEST: Basic Movement")
    print("=" * 40)
    
    sequences = [
        ("Vooruit", 50, 0),
        ("Stop", 0, 0),
        ("Achteruit", -50, 0),
        ("Stop", 0, 0),
        ("Links draaien", 0, 50),
        ("Stop", 0, 0),
        ("Rechts draaien", 0, -50),
        ("Stop", 0, 0),
    ]
    
    for name, t, s in sequences:
        print(f"  → {name}: T={t}, S={s}")
        await ws.send(make_command(t, s))
        await asyncio.sleep(1.0)
    
    # Hard stop
    await ws.send(make_command(0, 0))

async def test_speed_ramp(ws):
    """Test PWM ramp"""
    print("\n🧪 TEST: Speed Ramp")
    print("=" * 40)
    
    # Opbouwen
    for pwm in range(0, 101, 10):
        await ws.send(make_command(pwm, 0))
        await asyncio.sleep(0.2)
    
    # Afbouwen
    for pwm in range(100, -101, -10):
        await ws.send(make_command(pwm, 0))
        await asyncio.sleep(0.2)
    
    await ws.send(make_command(0, 0))

async def test_steering(ws):
    """Test sturing bij vaste snelheid"""
    print("\n🧪 TEST: Steering at Fixed Speed")
    print("=" * 40)
    
    await ws.send(make_command(30, 0))  # Vaste snelheid
    await asyncio.sleep(0.5)
    
    for steer in [-80, -40, 0, 40, 80, 0]:
        await ws.send(make_command(30, steer))
        print(f"  Steer: {steer}")
        await asyncio.sleep(1.0)
    
    await ws.send(make_command(0, 0))

async def test_mode_switch(ws):
    """Test mode switching"""
    print("\n🧪 TEST: Mode Switch")
    print("=" * 40)
    
    await ws.send(make_command(0, 0, 0))  # Manual
    print("  Mode: MANUAL")
    await asyncio.sleep(1)
    
    await ws.send(make_command(0, 0, 1))  # Auto
    print("  Mode: AUTO")
    await asyncio.sleep(1)
    
    await ws.send(make_command(0, 0, 0))  # Manual
    print("  Mode: MANUAL")

# ==================== TELEMETRY HANDLER ====================
async def telemetry_listener(ws, stop_event: asyncio.Event):
    """Luister continu naar telemetrie"""
    print("📡 Telemetry listener started")
    try:
        async for message in ws:
            data = parse_telemetry(message)
            if data:
                # Compact display
                t = data.get('t', 0)
                dF = data.get('dF', -1)
                dR = data.get('dR', -1)
                bat = data.get('bat', 0.0)
                imu = data.get('imu', {})
                enc = data.get('enc', {})
                line = data.get('line', 0.0)
                mode = data.get('mode', 0)
                estop = data.get('estop', False)
                
                print(f"\r📊 t={t}ms | dF={dF}cm dR={dR}cm | "
                      f"bat={bat}V | roll={imu.get('r',0)}° pitch={imu.get('p',0)}° "
                      f"| enc L={enc.get('L',0)} R={enc.get('R',0)} | "
                      f"line={line:.2f} | mode={mode} {'🛑 ESTOP' if estop else ''}", 
                      end="", flush=True)
    except asyncio.CancelledError:
        pass
    except Exception as e:
        print(f"\n❌ Telemetry error: {e}")

# ==================== INTERACTIVE CONTROL ====================
async def interactive_control(ws):
    """Handmatige controle via toetsenbord"""
    print("\n🎮 INTERACTIVE CONTROL")
    print("=" * 40)
    print("WASD = rijden | SPACE = stop | Q = quit")
    print("M = mode toggle | C = clear emergency | 1-9 = snelheid preset")
    print("-" * 40)
    
    throttle = 0
    steer = 0
    mode = 0
    speed_step = 10
    
    # Start telemetry listener
    stop_event = asyncio.Event()
    telemetry_task = asyncio.create_task(telemetry_listener(ws, stop_event))
    
    try:
        while True:
            # Non-blocking input check
            print("\n> ", end="", flush=True)
            cmd = await asyncio.get_event_loop().run_in_executor(None, sys.stdin.readline)
            cmd = cmd.strip().lower()
            
            if cmd == 'q':
                break
            elif cmd == 'w':
                throttle = min(100, throttle + speed_step)
            elif cmd == 's':
                throttle = max(-100, throttle - speed_step)
            elif cmd == 'a':
                steer = max(-100, steer - speed_step)
            elif cmd == 'd':
                steer = min(100, steer + speed_step)
            elif cmd == ' ':
                throttle = 0
                steer = 0
            elif cmd == 'm':
                mode = 1 - mode
                print(f"  Mode: {'AUTO (PID)' if mode else 'MANUAL'}")
            elif cmd == 'c':
                # Clear emergency stop
                await ws.send(make_command(0, 0, 0, estop_clear=True))
                print("  Sent: Emergency stop CLEAR")
                continue
            elif cmd in '123456789':
                throttle = int(cmd) * 10
            elif cmd == '0':
                throttle = 0
            elif cmd == 'h':
                print("WASD=move SPACE=stop M=mode C=clear estop 1-9=speed Q=quit")
                continue
            else:
                continue
            
            await ws.send(make_command(throttle, steer, mode))
            print(f"  Sent: T={throttle} S={steer} M={mode}")
            
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        telemetry_task.cancel()
        await ws.send(make_command(0, 0, 0))

# ==================== JOYSTICK CONTROL ====================
async def joystick_control(ws):
    """Joystick controle (vereist pygame)"""
    try:
        import pygame
    except ImportError:
        print("❌ pygame niet geïnstalleerd: pip install pygame")
        return
    
    pygame.init()
    pygame.joystick.init()
    
    if pygame.joystick.get_count() == 0:
        print("❌ Geen joystick gevonden")
        return
    
    joystick = pygame.joystick.Joystick(0)
    joystick.init()
    print(f"🎮 Joystick: {joystick.get_name()}")
    
    stop_event = asyncio.Event()
    telemetry_task = asyncio.create_task(telemetry_listener(ws, stop_event))
    
    try:
        while True:
            pygame.event.pump()
            
            # Axis 0 = links/rechts, Axis 1 = voor/achter
            steer_axis = joystick.get_axis(0)   # -1.0 .. 1.0
            throttle_axis = -joystick.get_axis(1)  # -1.0 .. 1.0 (omgekeerd)
            
            # Deadzone
            deadzone = 0.1
            if abs(steer_axis) < deadzone:
                steer_axis = 0
            if abs(throttle_axis) < deadzone:
                throttle_axis = 0
            
            # Map naar -100..100
            steer = int(steer_axis * 100)
            throttle = int(throttle_axis * 100)
            
            # Knoppen
            if joystick.get_button(0):  # A button = stop
                throttle = 0
                steer = 0
            if joystick.get_button(1):  # B button = mode toggle
                # TODO: mode toggle
                pass
            
            await ws.send(make_command(throttle, steer))
            
            await asyncio.sleep(0.05)  # 20Hz
            
    except KeyboardInterrupt:
        pass
    finally:
        stop_event.set()
        telemetry_task.cancel()
        await ws.send(make_command(0, 0))
        pygame.quit()

# ==================== MAIN ====================
async def main():
    parser = argparse.ArgumentParser(description="ESP32 Rover WebSocket Test Client")
    parser.add_argument("--ip", default=ROVER_IP, help=f"Rover IP (default: {ROVER_IP})")
    parser.add_argument("--port", type=int, default=ROVER_PORT, help=f"Port (default: {ROVER_PORT})")
    parser.add_argument("--auto", action="store_true", help="Run auto test sequence")
    parser.add_argument("--joystick", action="store_true", help="Joystick control")
    parser.add_argument("--interactive", action="store_true", help="Interactive keyboard control")
    args = parser.parse_args()
    
    ws_url = f"ws://{args.ip}:{args.port}{WS_PATH}"
    print(f"🔌 Connecting to {ws_url}...")
    
    try:
        async with websockets.connect(ws_url) as ws:
            print("✅ Connected!")
            
            # Wacht op eerste telemetrie
            try:
                msg = await asyncio.wait_for(ws.recv(), timeout=5.0)
                print(f"📥 First message: {msg[:100]}...")
            except asyncio.TimeoutError:
                print("⚠️ No initial telemetry received")
            
            if args.auto:
                await test_basic_movement(ws)
                await test_speed_ramp(ws)
                await test_steering(ws)
                await test_mode_switch(ws)
            elif args.joystick:
                await joystick_control(ws)
            elif args.interactive:
                await interactive_control(ws)
            else:
                # Default: interactive
                await interactive_control(ws)
                
    except ConnectionRefusedError:
        print(f"❌ Connection refused. Is rover AP running at {args.ip}?")
        print(f"   Check: WiFi connect to 'Rover_AP' password 'rover123'")
    except Exception as e:
        print(f"❌ Error: {e}")

if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        print("\n👋 Stopped by user")