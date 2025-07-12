<div align=center>
  <img src="https://github.com/Darkone83/LoveByte/blob/main/images/logo.jpg" height=400 width=400>
</div>

# LoveByte: The Cutest Little Messenger for Your Tech Life! 💌✨

<div align=center>
  <img src="https://github.com/Darkone83/LoveByte/blob/main/images/message.jpg" height=300 width=300>
</div>

Meet **LoveByte**, your adorable ESP32-powered sidekick! She’s not just a smart device—she’s your new BFF for sending love notes, sparkly messages, and cute pics right to your DIY gadgets. Here’s what she can do:

- **Wi-Fi Sweetheart**: She sets up her own Wi-Fi and invites you in with a friendly captive portal—no drama, just connect and play!
- **Cloud Messenger**: Type your thoughts, doodles, or encouragements in your web browser, and she’ll deliver them to your device friends (even if you’re in different rooms!).
- **Picture Princess**: Upload images and GIFs—she’ll resize and show them off on her stylish little screen for everyone to see.
- **LED Diva**: Change her colors or make her “heartbeat” with a glowing light, perfect for setting the mood or showing off your style.
- **File Organizer**: She keeps your files neat and tidy with a built-in file manager.
- **Status Star**: Peek at her diagnostics to see how she’s feeling—Wi-Fi, uptime, even the weather!
- **No Gatekeeping**: LoveByte makes messaging easy, skipping over complicated networks and techie hurdles.
- **Totally Customizable**: From her name to her colors, she’s all about letting you express yourself.

So whether you’re sending a sweet note to your game room, brightening up your desk, or just love adorable gadgets, **LoveByte** is here to keep your tech life charming and connected. 💖

---

## 💖 Required Hardware: Build Your LoveByte Dream Team! 💖

Before you start sprinkling your messages and cute pics everywhere, let’s make sure you have everything you need for your LoveByte adventure! Don’t worry—she’s not high-maintenance, just a little fabulous. ✨

### 🌸 What You’ll Need:

- **ESP32-S3 Touch LCD 1.47” Board**  
  She’s the heart of your LoveByte setup—compact, colorful, and oh-so-smart!  
  [✨ Get it on Amazon! ✨](https://www.amazon.com/dp/B0FBWJ6KXH?ref_=ppx_hzsearch_conn_dt_b_fed_asin_title_6)

- **MicroSD Card**  
  For keeping your messages, pics, and all the good stuff safe and sound.  
  Any standard microSD card will do—give her a little extra space to shine!

- **USB-C Cable**  
  For power and programming. Bonus points if it’s pink! 🌷

- **Wi-Fi Network**  
  So LoveByte can chat and sparkle with all her friends.

That’s it! Gather your supplies, put on your favorite playlist, and get ready to give your LoveByte a home she’ll adore.  
If you need help, just reach out—LoveByte loves making new friends! 💌

---

## 💾 Setting Up Your LoveByte SD Card: Give Her a Little Space to Shine! ✨

Every diva needs her own closet—and LoveByte is no different! Your microSD card is where she keeps all her precious files, memories, and that dazzling splash screen when she powers up.

### 🛍️ What Goes Where?

After you’ve got your microSD card (any size is fine—LoveByte isn’t greedy!), you’ll want to make sure it’s set up just right:

- **images/** — This is where LoveByte saves and finds her received pictures and GIFs.
- **messages/** — All your sweet messages, perfectly organized and saved.
- **res/** — Special things live here, like her stylish **splash.jpg** (that cute logo you see at startup).

### 🎀 Step-by-Step Setup:

1. **Format your microSD card** (FAT32 recommended—she likes things tidy!).
2. **Make these three folders** at the top level of the card:  
   - `images`
   - `messages`
   - `res`
3. **Add your splash screen**:  
   Place your custom <a href="https://github.com/Darkone83/LoveByte/tree/main/sdcard/res">`splash.jpg`</a> into the `res` folder.  
   That’s the image LoveByte shows off when she boots up—so make it cute!

### 💡 Pro Tips

- Want to change the splash? Just swap out `splash.jpg` anytime.
- You can add, remove, or peek at images and messages anytime using the file manager in the web UI.

With your microSD card set up, LoveByte has everything she needs to keep your memories safe and start every day with a little extra sparkle. 🌟

---

## 🛠️ Arduino Setup: Libraries Every LoveByte Needs! 📚✨

Before you can bring your LoveByte to life, you’ll need to make sure her wardrobe is fully stocked with all the right libraries. These are the must-have accessories that make everything run smoothly—think of them as her favorite shoes, purse, and a sparkly necklace!

### 💅 Required Arduino Libraries

Head over to the Arduino Library Manager and add these gems:

- **ESPAsyncWebServer**  
- **AsyncTCP**  
- **ArduinoJson**    
- **AnimatedGIF** 
- **LovyanGFX**  

### 💖 How To Install

**In Arduino IDE:**  
   - Go to `Tools` → `Manage Libraries…`
   - Search for each library name above and click “Install”.

Now your LoveByte has everything she needs to sparkle, connect, and express herself! If you’re not sure which driver to pick for your display, check the board details or just ask—LoveByte loves to help her friends look their best. 💃

---

## 🚀 Compiling & Flashing: Bringing Your LoveByte to Life! ✨

You’re almost there! Once all the libraries are in place and your code is looking cute, it’s time for the magic moment: compiling and flashing your LoveByte to her new home.

### 💻 Quick Steps

1. **Plug in your LoveByte board** with a USB-C cable.
2. **Open the project** in Arduino IDE or PlatformIO.
3. **Select your board** (`ESP32S3 Dev Module` or the exact match for your LoveByte device).
4. **Choose the correct COM port**—so she knows where to land.
5. **Board options**
<div align=center>
  <img src="https://github.com/Darkone83/LoveByte/blob/main/images/arduino.png">
</div>
6. **Click “Upload”** (the right-facing arrow) and let the sparkle happen!

She’ll wiggle her way into your heart (and her new board) in no time.  
When she restarts, she’s ready to dazzle and connect—just like the social butterfly she is!

If she’s shy and doesn’t show up right away, double-check your USB cable and port settings. Sometimes even divas need a second try! 💖

---

## 🌐 First-Time Setup & Accessing Your LoveByte’s Pages! 💖

Once your LoveByte is powered up for the first time, she’ll roll out the welcome mat with a Wi-Fi hotspot just for you!

### ✨ Getting Started

1. **Join LoveByte’s Wi-Fi**  
   - On first boot, connect your phone or computer to the “LoveByte” Wi-Fi network she creates.
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/network.png" height=250 width=300>
  </div>

2. **Open Your Browser**  
   - Navigate to [http://192.168.4.1](http://192.168.4.1) to set up your Wi-Fi.  
   - Follow the prompts to connect LoveByte to your home network.
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/wifi.png" height=650 width=250>
  </div>

3. **Find Her New Address**  
   - Once connected, open your browser and visit `lovebyte.local`.

---

## 🌦️ Weather Setup: Give LoveByte Her Daily Forecast! ☀️🌧️

Want LoveByte to greet you with the weather each morning or whenever you check in? She can, but she’ll need a little help from you (and OpenWeather)!

1. **Get Your OpenWeather API Key**  
   - Visit [https://openweathermap.org/api](https://openweathermap.org/api) and sign up for a free account.
   - Once logged in, create an API key (it’s super quick!).

2. **Enter the API Key on LoveByte’s Config Page**  
   - Go to `http://[LoveByte_IP]/config` in your browser.
   - Paste your API key into the weather section.
   - Set your city and country code.

3. **Save Your Changes**  
   - Click “Save” and give LoveByte a moment to update.
   - She’ll now fetch and show your local weather with every new message or screen refresh!

*Now your LoveByte can dress for the weather—maybe even bring an umbrella emoji on rainy days!* ☔🌸


### 💕 Exploring the Pages

Now you’re ready to visit LoveByte’s web pages!

- **Landing Page:**  
  `http://lovebyte.local/lb`  
  See her style and options.
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/landing.png" height=650 width=250>
  </div>

- **Message Center:**  
  `http://lovebyte.local/lb/cloud`  
  Send messages, images, and all the good vibes.
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/message.png" height=650 width=250>
  </div>

- **Configuration:**  
  `http://lovebyte.local/config`  
  Update her name, server address, and settings.
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/config.png" height=650 width=250>
  </div>

- **File Manager:**  
  `http://lovebyte.local/files`  
  Upload or organize your images and messages.
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/fileman1.png" height=650 width=250>

    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/fileman2.png" height=650 width=250>
  </div>

- **Diagnostics:**  
  `http://lovebyte.local/diag`  
  Peek at her vitals—Wi-Fi, uptime, memory, and more!
  <div align=center>
    <img src="https://github.com/Darkone83/LoveByte/blob/main/images/diag.png" height=650 width=250>
  </div>

Bookmark your favorites—she loves visitors!

Now your LoveByte is online and ready to shine! 🎀✨

---
