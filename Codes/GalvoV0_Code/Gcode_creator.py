import cv2
import math
import os



def createGCode(path, x, pixelsize, threshold, feed, jog):
    """Creates Gcode. Uses desired horizontal width (mm) for picture to scale."""

    def calc(val):
        return round(val*pixelsize, 2)

    img = cv2.imread(path, cv2.IMREAD_GRAYSCALE)
    r = img.shape[1]/img.shape[0]
    cv2.imshow("wrench", img)
    columns = math.ceil(x/pixelsize)
    rows = math.ceil(columns * r)
    resized = cv2.resize(img, (columns, rows), interpolation=cv2.INTER_AREA)
    resized = cv2.threshold(resized, threshold, 255, cv2.THRESH_BINARY)[1]
    #cv2.imshow("wrench", img)
    #cv2.imshow("wrench, resized", resized)
    #cv2.waitKey()

    if os.path.exists("laser_gcode.txt"):
        os.remove("laser_gcode.txt")
    f = open("laser_gcode.txt", "w+")
    f.write("G28\r")
    f.write("G0 X0 Y0 F%d\r" % feed)
    laser_on=False
    for i in range(rows):  # y
        prev_val = 255
        if laser_on == True:
            f.write("M5\r")
            laser_on = False
        f.write("G0 X0 Y-{} F{}\r".format(calc(i), jog))
        for j in range(columns):   # x
            if prev_val == 255 and resized[i, j] == 0:
                if j==columns-1:
                    f.write("G0 X{} Y-{} F{}\r".format(calc(j), calc(i), jog))
                    f.write("M6\r")
                elif resized[i,j+1] == 255:
                    f.write("G0 X{} Y-{} F{}\r".format(calc(j), calc(i), jog))
                    f.write("M6\r")
                else:
                    f.write("G0 X{} Y-{} F{}\r".format(calc(j), calc(i), jog))
                    f.write("M3\r")
                    laser_on = True
                prev_val = 0
            elif prev_val == 0 and resized[i, j] == 255:
                f.write("G0 X{} Y-{} F{}\r".format(calc(j-1), calc(i), feed))
                if laser_on == True:
                    f.write("M5\r")
                    laser_on = False
                prev_val = 255
    f.close()


#createGCode("wrench.png", 20, 0.2, 128, 30)
