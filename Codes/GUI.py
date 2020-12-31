import tkinter as tk
import os
import tkinter.filedialog as fd
import tkinter.font as font
import Gcode_creator as gc
import Gcode_handler as gh
from PIL import ImageTk, Image
import serial
import time

class GUI:
    port = ""
    ser = None

    def __init__(self, master):
        self.master = master
        master.title("GalvoStepV1")
        master.geometry("1200x800")

        titlefont = font.Font(family='Helvetica', size=20)
        subtitlefont = font.Font(family='Helvetica', size=15)
        labelfont = font.Font(family='Helvetica', size=13)
        buttonfont = font.Font(family='Helvetica', size=10)

        self.label = tk.Label(master, text="GalvoStepV1 Suite", font=titlefont)
        self.label.grid(row=0, column=3)
        self.label = tk.Label(master, text="GCode Creator", font=subtitlefont)
        self.label.grid(row=1, column=1)
        self.label = tk.Label(master, text="Machine Operation", font=subtitlefont)
        self.label.grid(row=1, column=4)
        self.label2 = tk.Label(master, text="Enter image path:", font=labelfont)
        self.label2.grid(row=2, column=0)
        self.pathentry = tk.Entry(master, text="image path", width=35, font=labelfont)
        self.pathentry.grid(row=2, column=1)
        self.pathbutton1 = tk.Button(master, text="Browse", command=self.browse, pady=10, padx=30, font=buttonfont)
        self.pathbutton1.grid(row=2, column=2)
        self.connectbutton = tk.Button(master, text="Connect", command=self.connect, pady=12, padx=40, font=subtitlefont)
        self.connectbutton.grid(row=2, column=4)

        self.xlabel = tk.Label(master, text="x length (mm)", width=18, font=labelfont)
        self.xlabel.grid(row=3, column=0)
        self.pixlabel = tk.Label(master, text="laser dot size(mm)", width=18, font=labelfont)
        self.pixlabel.grid(row=3, column=1)
        self.feedlabel = tk.Label(master, text="Feedrate (steps/sec)", width=18, font=labelfont)
        self.feedlabel.grid(row=3, column=2)
        self.joglabel = tk.Label(master, text="Jograte (steps/sec)", width=18, font=labelfont)
        self.joglabel.grid(row=5, column=2)
        self.serialbutton = tk.Button(master, text="Send Command", command=self.send_serial, state='disabled', pady=10, padx=30, font=buttonfont)
        self.serialbutton.grid(row=4, column=3)
        self.jobbutton = tk.Button(master, text="Start Job", command=self.start_job, state='disabled', pady=10, padx=30, font=buttonfont)
        self.jobbutton.grid(row=5, column=3)

        self.machinestatus = tk.Label(master, text="MACHINE STATUS", width=18, font=labelfont)
        self.machinestatus.grid(row=3, column=3)
        self.serialentry = tk.Entry(master, font=labelfont)
        self.serialentry.grid(row=4, column=4)

        self.xentry = tk.Entry(master, width=15, font=labelfont)
        self.xentry.grid(row=4, column=0)
        self.xentry.insert(10, "20")
        self.pixentry = tk.Entry(master, text="laser dot size(mm)", width=15, font=labelfont)
        self.pixentry.grid(row=4, column=1)
        self.pixentry.insert(10, "0.2")
        self.feedentry = tk.Entry(master, text="Feedrate (steps/sec)", width=15, font=labelfont)
        self.feedentry.grid(row=4, column=2)
        self.feedentry.insert(10, "40")
        self.jogentry = tk.Entry(master, text="Jograte (steps/sec)", width=15, font=labelfont)
        self.jogentry.grid(row=6, column=2)
        self.jogentry.insert(10, "200")
        self.comentry = tk.Entry(master, text="COM Port", width=10, font=labelfont)
        self.comentry.grid(row=2, column=3)
        self.comentry.insert(10, 'COM3')

        self.messagelabel = tk.Label(master, text="Enter the image path and process variables...", font=labelfont)
        self.messagelabel.grid(row=7, column=1)
        self.machinelabel = tk.Label(master, text="Disconnected...", font=labelfont, bd=2, relief="solid")
        self.machinelabel.grid(row=3, column=4)

        self.pathbutton2 = tk.Button(master, text="Create GCode", command=self.create_gcode, pady=15, padx=30, font=buttonfont)
        self.pathbutton2.grid(row=8, column=0)
        self.pathbutton2 = tk.Button(master, text="Check Picture", command=self.check_img, pady=15, padx=30, font=buttonfont)
        self.pathbutton2.grid(row=8, column=2)
        self.close_button = tk.Button(master, text="Close", command=master.quit, pady=12, padx=40, font=subtitlefont)
        self.close_button.grid(row=9, column=2)


    def check_img(self):
        try:
            load = Image.open(self.pathentry.get())
            img = ImageTk.PhotoImage(load)
            labelImg = tk.Label(self.master, image=img)
            labelImg.image = img
            labelImg.grid(row=8, column=1)
        except AttributeError:
            self.messagelabel['text'] = "Please enter a valid path"

    def browse(self):
        self.pathentry.delete(0, 'end')
        tempdir = fd.askopenfilename(parent=root, initialdir=os.getcwd(), title='Please select a directory')
        self.pathentry.insert(tk.END, tempdir)

    def create_gcode(self):
        try:
            gc.createGCode(self.pathentry.get(), x=float(self.xentry.get()), pixelsize=float(self.pixentry.get()),
                           threshold=128, feed=float(self.feedentry.get()), jog=float(self.jogentry.get()))
            if os.path.exists("laser_gcode.txt"):
                self.messagelabel['text'] = "GCode file was created."
        except ValueError:
            self.messagelabel['text'] = "Please enter a number for the process variables"
        except AttributeError:
            self.messagelabel['text'] = "Please enter a valid path"

    def connect(self):
        try:
            self.port = self.comentry.get()
            self.ser = serial.Serial(self.port, 115200)
            self.ser.flushInput()
            self.machinelabel['text'] = "Connected"
            self.serialbutton['state'] = 'normal'
            self.jobbutton['state'] = 'normal'
        except serial.serialutil.SerialException:
            self.machinelabel['text'] = "Please select a correct COM Port"

    def send_serial(self):
        try:
            cmd = self.serialentry.get()
            feedback = gh.send_command(cmd, self.ser)
            self.machinelabel['text'] = feedback
        except serial.serialutil.SerialException:
            self.machinelabel['text'] = "Machine is not connected"


    def start_job(self):
        try:
            file = open("laser_gcode.txt", "r")
            self.machinelabel['text'] = "Job started"
            time.sleep(2)
            self.machinelabel['text'] = "Homing..."
            for line in file:
                self.ser.write(str.encode(line.strip() + "\n"))
                feedback = self.ser.readline()
                self.machinelabel['text'] = feedback

            self.machinelabel['text'] = "Job done"
            time.sleep(0.5)
            file.close()
        except serial.serialutil.SerialException:
            self.machinelabel['text'] = "Machine is not connected"
        

root = tk.Tk()
my_gui = GUI(root)
root.mainloop()
