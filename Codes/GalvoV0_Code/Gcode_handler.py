import serial
import time



def start_job(file_path, port):
    ser = serial.Serial(port, 115200)

    file = open(file_path, "r")
    print("Connected")
    time.sleep(2)
    ser.flushInput()
    #_ = ser.readline()
    print("Homing...")
    for line in file:
        l = line.strip()
        ser.write(str.encode(l+"\n"))
        _ = ser.readline()
        print(_)

    print("GCODE SENT")
    time.sleep(0.5)
    file.close()
    ser.close()


def send_command(command, ser):
    l = command.strip()
    ser.write(str.encode(l + "\n"))
    _ = ser.readline()
    return _



#start_job("Calibration_test", 'COM8')