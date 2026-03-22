import pyautogui
import time
import mysql.connector

mydb = mysql.connector.connect(
  host="192.168.43.146",
  user="quan",
  password="27122001",
  database="flight_sim"
)

mydb.autocommit = True

pyautogui.FAILSAFE = False

x_center = 960/4
y_center = 565/4

pyautogui.moveTo(x_center, y_center)

prev_pot_value=0


while(True):
	mycursor = mydb.cursor()
	mycursor.execute("SELECT x_coor,y_coor,pot,le, ri,up FROM table1 ORDER BY Number DESC LIMIT 1")
	x ,y, raw_adc, left, right, flap = mycursor.fetchone()

	pot_value = int(raw_adc/293)



	print(x,y,pot_value, left, right, flap)
	

	#control cursor
	pyautogui.moveTo(x_center*4+(0-y)*2, y_center*4-(35+x)*3)	

	#potentiometer
	if(pot_value - prev_pot_value) > 0:
		for i in range (pot_value - prev_pot_value):
			pyautogui.keyDown('pageup')
			pyautogui.keyUp('pageup')

	if(pot_value - prev_pot_value) < 0:
		for i in range (abs(pot_value - prev_pot_value)):
			pyautogui.keyDown('pagedown')
			pyautogui.keyUp('pagedown')
	prev_pot_value = pot_value

	#left right
	if left!=0:
		pyautogui.keyDown(',')
	else:
		pyautogui.keyUp(',')
		
		
	if right!=0:
		pyautogui.keyDown('.')
	else:
		pyautogui.keyUp('.')

	if flap!=0:
		pyautogui.keyDown('f')
		pyautogui.keyUp('f')

		

	

