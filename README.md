# AutoPot
<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Menacing_stare.jpeg"/>

## What is this?

This is an advanced smart pot for a plant. It has everything other smart planters have. What is the difference between the AutoPot and a regular smart planter? It has all the features all different types of smart planters have and combines them into one pot! This means that it is the ultimate pot.

### Features
  - Automatic care for the plant
  - Manual care for the plant
  - Interaction with pet
  - Customization in the pot itself
  - Remote access through website
  - Low power usage
  - Cool look

---

## Why?

I always wanted to get a desktop plant, but my forgetfulness would get in my way. Because of that, I would have multiple plants that would die under my care 💀. Still, I wanted a plant. Smart planters were expensive and the "Stardance" event was happening so I decided it was the perfect moment to try something new. Additionally, I found that smart pots have features where no one pot has all the features so this was my chance.

---

## Extra

I used Tinkercad for my 3D modelling. I am a noob at this stuff so I started with the basics. The basics did suffice. And for the code I used the Arduino IDE. At first trying to code and do the circuitry was difficult, but I was able to persevere. It was mostly because of the coding. I had known how to code but that skill was dormant for so long that I literally forgot. Similarly, my electronics skill had gotten worse. Fortunately, through this experience I was able to get them back to form.

---

## 3D model work

<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/CAD_view.png"/>
<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/CAD_handle2.png"/>
<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Cap_bottle.jpeg"/>
<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Drawer_view.jpeg"/>
<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Top_view.jpeg"/>
My skills are definitely not the best but I think it turned out nice. I had to make 3 different files for this one. The first one is the main file where I have the base alongside the cap and the screw. The second one is the file for the handle for the LED lights. Here, I had to make holes in the 3D model cuz I figured since it's going to be on for an extended amount of time that we need as much heat dissipation. Finally, I have a separate file for the drawer. FYI I had to make a separate cap for the bottles cuz I needed a tube to go through them and I have a small in it so that air can go in. 

---

## Wire work

So working with wires was a bit more interesting here. I was able to make a circuit before the orders had come so yeah. This is how it had looked. It was pretty easy tbh but not lemon squeezy.

<img src="https://github.com/Oriprogram/AutoPot/blob/main/Everything%20else/Component%20diagram.png"/>

Wire Jumpscare:

<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Jumpscare.jpeg"/>

---

## Icon/Face of the project
<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Icon.jpeg"/>

---

## Website

I made the website pretty easily. I mean I didn't code it. I prompted. It took a couple of versions until I landed with one that I did kinda like. It is very bare bones which I thought would be important.

<img src="https://github.com/Oriprogram/AutoPot/blob/main/Images/Website.png"/>

## Google Firebase

I used Google Firebase Realtime Database to connect the pot to the web app. Every few seconds, the ESP32 sends soil moisture, sunlight levels, and the plant's mood to Firebase over HTTPS, which updates the phone app in real time. It also works in reverse. When you tap a button on your phone to turn on the grow light or trigger the water pump, Firebase sends that command down to the ESP32 so it can switch the relays. Google Auth is also used so each pot stays linked to its owner.

---

## BOM

| Part                                          | Purpose              | Cost   | Quantity     | Total Cost  | Link                                                                |
| --------------------------------------------- | -------------------- | ------ | ------------ | ----------- | ------------------------------------------------------------------- |
| ESP32-2432S028R                               | Computer and Display | $7.95  | 1            | $7.95       | [Pinduoduo](https://mobile.yangkeduo.com/goods1.html?ps=I6NX7x8Ii4) |
| Capacitive Soil Moisture Sensor v1.2          | Soil sensor          | $0.90  | 1            | $0.90       | [Pinduoduo](https://mobile.yangkeduo.com/goods.html?ps=vO3mJ54ga6)  |
| Diaphragm Pump                                | Pump                 | $0.75  | 1            | $0.75       | [Pinduoduo](https://mobile.yangkeduo.com/goods.html?ps=oJcak3aJid)  |
| Direct Charging port Type-C                   | USB port             | $0.375 | 2            | $0.75       | [Pinduoduo](https://mobile.yangkeduo.com/goods.html?ps=E8jgiR4MY5)  |
| 170-point Breadboard                          | Breadboard           | $0.30  | 1            | $0.30       | [Pinduoduo](https://mobile.yangkeduo.com/goods1.html?ps=WdkfbvcOKr) |
| 60 pieces of wires (20 mm, 20 mf, 20 ff)      | Wires                | $0.90  | 60 (1 pack)  | $0.90       | [Pinduoduo](https://mobile.yangkeduo.com/goods2.html?ps=DawUrRCinH) |
| 4MM ID and 6MM ED plastic tube                | Tubing               | $0.30  | 1 meter      | $0.30       | [Pinduoduo](https://mobile.yangkeduo.com/goods.html?ps=YnGHx4lljj)  |
| Standard USBA to USBC data cable              | USB cable            | $0.75  | 1            | $0.75       | [Pinduoduo](https://mobile.yangkeduo.com/goods.html?ps=eStvAQ7Vt8)  |
| Pack of zip ties                              | Zip ties             | $0.30  | 100 (1 pack) | $0.30       | [Pinduoduo](https://mobile.yangkeduo.com/goods2.html?ps=RwF3GCbRdx) |
| Double-sided tape                             | Double-sided tape    | $0.15  | 1 meter      | $0.15       | [Pinduoduo](https://mobile.yangkeduo.com/goods1.html?ps=j4KT3ygYmg) |
| COB LED                                       | LED                  | $1.875 | 20 cm        | $1.875      | [Pinduoduo](https://mobile.yangkeduo.com/goods1.html?ps=QTPc1W1zjk) |
| 2-channel 5V Relay Module                     | Relay Module         | $1.50  | 1            | $1.50       | [Pinduoduo](https://mobile.yangkeduo.com/goods1.html?ps=5oHHM1r4d8) |
| Standard 5V 2.4A charging block               | Power Block          | $1.35  | 1            | $1.35       | [Pinduoduo](https://mobile.yangkeduo.com/goods1.html?ps=OMG2KobXzv) |
| 1.25mm 4 port cable                           | CYD cable            | $0.15  | 10           | $1.50       | [Pinduoduo](https://mobile.yangkeduo.com/goods.html?ps=NVVRROu08q)  |
| Custom 3D Printing Service (Chassis & Drawer) | 3D print             | $28.00 | 1            | $28.00      | [Lalafo](https://lalafo.kg/)                                        |
| **TOTAL**                                     |                      |        |              | **$47.275** |                                                                     |

---

## End

So this is the end of my project. It was my first time actually trying to finish a project from start to finish. And it was also my first time doing a project individually. It was really fun and I kinda liked it. Hopefully, after my university application stuff ends and I get into a university, I can start another solo project. Until then, I am happy I got to be able to participate in Stardance. I really appreciate it. And this means a lot to me. I have finally finished the project.

Thanks <3
