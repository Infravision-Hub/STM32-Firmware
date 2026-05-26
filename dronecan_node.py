import dronecan

node = dronecan.make_node('PCAN_USBBUS1', node_id=122, bitrate=1000000, bustype='pcan')
node.node_info.name = "com.test.node"
node.node_info.software_version.major = 1
node.node_info.software_version.minor = 0
node.node_info.hardware_version.major = 1
node.mode = dronecan.uavcan.protocol.NodeStatus().MODE_OPERATIONAL
node.health = dronecan.uavcan.protocol.NodeStatus().HEALTH_OK

v1 = v2 = v3 = v4 = 0.0
c1 = c2 = c3 = c4 = 100.0
load_cell = 0.0
encoder = 100.0

def make_batt_msg(battery_id, voltage, current):
    msg = dronecan.uavcan.equipment.power.BatteryInfo()
    msg.voltage = voltage
    msg.current = current
    msg.remaining_capacity_wh = 100.0
    msg.full_charge_capacity_wh = 100.0
    msg.state_of_charge_pct = 50
    msg.battery_id = battery_id
    return msg

def send_batt1():
    global v1, c1
    v1 += 1
    c1 -= 1
    node.broadcast(make_batt_msg(0, v1, c1))
    print(f"[TX] Batt1 voltage={v1} current={c1}")

def send_batt2():
    global v2, c2
    v2 += 2
    c2 -= 2
    node.broadcast(make_batt_msg(1, v2, c2))
    print(f"[TX] Batt2 voltage={v2} current={c2}")

def send_batt3():
    global v3, c3
    v3 += 3
    c3 -= 3
    node.broadcast(make_batt_msg(2, v3, c3))
    print(f"[TX] Batt3 voltage={v3} current={c3}")

def send_batt4():
    global v4, c4
    v4 += 4
    c4 -= 4
    node.broadcast(make_batt_msg(3, v4, c4))
    print(f"[TX] Batt4 voltage={v4} current={c4}")

def send_batt5():
    global load_cell, encoder
    load_cell += 5
    encoder -= 5
    node.broadcast(make_batt_msg(4, load_cell, encoder))
    print(f"[TX] Batt5 load_cell={load_cell} encoder={encoder}")

node.periodic(1.0, send_batt1)
node.periodic(1.1, send_batt2)
node.periodic(1.2, send_batt3)
node.periodic(1.3, send_batt4)
node.periodic(1.4, send_batt5)

while True:
    try:
        node.spin(timeout=0.1)
    except KeyboardInterrupt:
        break