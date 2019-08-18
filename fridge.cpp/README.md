Description of program:
Program to monitor two products in the fridge
Program needs 2 variables: brutto weight and netto weight
We have two sensor scales with HX711 connected to arduino
WiFi connection allows to download time from the Internet (NTP) and send PUSH messages to an android phone with the current weight status
For sending messeges PUSH I am usibg pushingbox.com, for reciving I am using Pushbullet app
According to the assumption, the message will be sent:
- everyday at 16:00
- immediately when a product with 1% -20% content appears on the scale
- after an hour when the weight is below 1% of content or empty (to prevent messages when you are just using the product)

Additionaly OLED display is added, and has functions:
- information about problem with connection
- printing weight status in % and brutto weight of both scales

To do:
- create a function to prepare push messages (not working after adding OLED and separating everything into functions)
- put send message conditions into functions
- put reset flags into functions
- change connecting - after 3 resets start working anyway, just put information "no WiFi" on OLED (now when there is no connection it resets after every 10 attempts)


If everything will be done, rotary encoder will be added with functions:
- read brutto weight and put it in variable (1 and 2)
- enter netto weight and put it in variable (1 and 2)
- tare scale (1 and 2)
- reset arudino
