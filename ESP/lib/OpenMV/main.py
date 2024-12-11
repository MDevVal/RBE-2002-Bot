import sensor, time, math, struct, omv
import pyb
uart = pyb.UART(1, 115200, timeout_char = 50)
uart.write("Hej, verden!!\n")
sensor.reset()
sensor.set_pixformat(sensor.GRAYSCALE)
if omv.board_type() == "H7": sensor.set_framesize(sensor.QQVGA)
elif omv.board_type() == "M7": sensor.set_framesize(sensor.QQVGA)
else: raise Exception("You need a more powerful OpenMV Cam to run this script")
sensor.skip_frames(time = 100)
sensor.set_auto_gain(False)
sensor.set_auto_whitebal(False)
clock = time.clock()
def checksum(data):
	checksum = 0
	for i in range(0, len(data), 2):
		checksum += ((data[i+1] & 0xFF) << 8) | ((data[i+0] & 0xFF) << 0)
	return checksum & 0xFFFF
def to_object_block_format(tag):
	angle = int((tag.rotation * 180) / math.pi)
	temp = struct.pack("<hhhhhh", tag.id, tag.cx, tag.cy, tag.w, tag.h, angle)
	return struct.pack("<bbh12sb", 0xFF, 0x55, checksum(temp), temp, 0xAA)
while(True):
	clock.tick()
	img = sensor.snapshot()
	tag_list = img.find_apriltags()
	for tag in tag_list:
		img.draw_rectangle(tag.rect, color=(255, 0, 0))
		img.draw_cross(tag.cx, tag.cy)
		for c in tag.corners:
			img.draw_circle(c[0], c[1], 5)
		print("Tag:", tag.id, tag.cx, tag.cy, tag.h, tag.w, tag.rotation)
		data_buf = to_object_block_format(tag)
		uart.write(data_buf)