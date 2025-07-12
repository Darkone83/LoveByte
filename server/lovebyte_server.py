import sys
import subprocess
import threading
import os

def ensure_dependencies():
    import importlib
    import pkg_resources
    required = {'flask', 'requests', 'pillow'}
    installed = {pkg.key for pkg in pkg_resources.working_set}
    missing = required - installed
    if missing:
        python = sys.executable
        subprocess.check_call([python, '-m', 'pip', 'install', *missing])
ensure_dependencies()

import tkinter as tk
from tkinter import ttk, messagebox, colorchooser, filedialog
from flask import Flask, request, jsonify, send_from_directory, after_this_request
import requests
import queue
import time
from PIL import Image, ImageSequence

# --- Panel Resolution (set from settings.h) ---
PANEL_WIDTH = 320   # CHANGE to your panel width
PANEL_HEIGHT = 170  # CHANGE to your panel height

app = Flask(__name__)
PORT = 6969

devices = {}  # device_id (lowercase): {'last_seen': float, 'messages': [msg, ...]}
message_queue = queue.Queue()

IMAGE_DIR = os.path.abspath("images")
if not os.path.exists(IMAGE_DIR):
    os.makedirs(IMAGE_DIR)

def clean_id(idstr):
    if idstr is None:
        return ""
    return idstr.strip().lower()

@app.route('/api/checkin', methods=['POST'])
def checkin():
    data = request.json
    device_id = clean_id(data.get('device_id'))
    print(f"[CHECKIN] device_id='{device_id}' (RAW: '{data.get('device_id')}')")
    if not device_id:
        return jsonify({'error': 'Missing device_id'}), 400
    devices.setdefault(device_id, {'last_seen': 0, 'messages': []})
    devices[device_id]['last_seen'] = time.time()
    print(f"[CHECKIN] Devices now: {list(devices.keys())}")
    return jsonify({'status': 'ok'})

@app.route('/api/pull', methods=['POST'])
def pull():
    data = request.json
    device_id = clean_id(data.get('device_id'))
    print(f"[PULL] device_id='{device_id}' (RAW: '{data.get('device_id')}')")
    if not device_id or device_id not in devices:
        print(f"[PULL] ERROR: device_id '{device_id}' not found in devices")
        return jsonify({'error': 'Unknown device_id'}), 404
    messages = devices[device_id]['messages']
    devices[device_id]['messages'] = []
    print(f"[PULL] Returning {len(messages)} messages for {device_id}")
    return jsonify({'messages': messages})

@app.route('/api/push', methods=['POST'])
def push():
    data = request.json
    recipient = clean_id(data.get('recipient'))
    print(f"[PUSH] recipient='{recipient}' (RAW: '{data.get('recipient')}'), available={list(devices.keys())}")
    if not recipient or recipient not in devices:
        print(f"[PUSH] ERROR: recipient '{recipient}' not found in devices")
        return jsonify({'error': 'Unknown recipient'}), 404

    def hexstr(val):
        if isinstance(val, str):
            return val.lstrip("#").upper()
        elif isinstance(val, int):
            return f"{val:06X}"
        else:
            return ""

    ledcolor = hexstr(data.get('ledColor', ""))
    hbcolor = hexstr(data.get('heartbeatColor', ""))

    msg = {
        'text': data.get('text', ''),
        'sender': data.get('sender', 'LoveByte server'),
        'time': data.get('time', ''),
        'weather': data.get('weather', ''),
        'city': data.get('city', 'Modesto'),
        'country': data.get('country', 'US'),
        'tempF': data.get('tempF', 0),
        'ledColor': ledcolor,
        'useLedColor': bool(data.get('useLedColor', False)),
        'useHeartbeat': bool(data.get('useHeartbeat', False)),
        'heartbeatColor': hbcolor,
        'heartbeatPulses': int(data.get('heartbeatPulses', 0))
    }
    print(f"[PUSH] Appending message to '{recipient}': {msg}")
    devices[recipient]['messages'].append(msg)
    message_queue.put({'action': 'log', 'msg': f"Sent to {recipient}: {msg['text']} (as message)"})
    return jsonify({'status': 'queued'})

@app.route('/api/upload_image', methods=['POST'])
def upload_image():
    file = request.files.get('file')
    recipient = clean_id(request.form.get('recipient'))
    if not file or not recipient:
        return jsonify({'error': 'Missing image or recipient'}), 400

    filename = file.filename.lower()
    base, ext = os.path.splitext(filename)
    ext = ext.lstrip('.')
    timestr = time.strftime("%Y%m%d_%H%M%S")
    if ext == "gif":
        out_file = os.path.join(IMAGE_DIR, f"img_{timestr}.gif")
    else:
        out_file = os.path.join(IMAGE_DIR, f"img_{timestr}.jpg")
    file.save(out_file)
    try:
        if ext in ["jpg", "jpeg", "png"]:
            img = Image.open(out_file)
            img = img.convert("RGB")
            img = img.resize((PANEL_WIDTH, PANEL_HEIGHT), Image.LANCZOS)
            img.save(out_file, "JPEG")
            result = "JPG/PNG resized"
        elif ext == "gif":
            gif = Image.open(out_file)
            frames = []
            for frame in ImageSequence.Iterator(gif):
                fr = frame.convert("RGBA")
                fr = fr.resize((PANEL_WIDTH, PANEL_HEIGHT), Image.LANCZOS)
                frames.append(fr)
            frames[0].save(out_file, save_all=True, append_images=frames[1:], loop=0, duration=gif.info.get("duration", 60))
            result = "GIF resized"
        else:
            return jsonify({'error': 'Unsupported image format'}), 400
    except Exception as e:
        return jsonify({'error': f'Image processing failed: {e}'}), 500

    msg = {
        'text': f"[IMAGE]{os.path.basename(out_file)}",
        'sender': 'LoveByte server',
        'time': time.strftime("%Y-%m-%d %H:%M:%S"),
        'weather': '',
        'city': 'Modesto',
        'country': 'US',
        'tempF': 0,
        'ledColor': "",
        'useLedColor': False,
        'useHeartbeat': False,
        'heartbeatColor': "",
        'heartbeatPulses': 0
    }
    devices[recipient]['messages'].append(msg)
    message_queue.put({'action': 'log', 'msg': f"Sent to {recipient}: {os.path.basename(out_file)} (as image)"})

    return jsonify({'status': 'uploaded', 'file': os.path.basename(out_file), 'result': result})

@app.route('/images/<filename>')
def get_image(filename):
    from flask import after_this_request
    path = os.path.join(IMAGE_DIR, filename)
    if not os.path.exists(path):
        return "Not found", 404
    @after_this_request
    def remove_file(response):
        try:
            os.remove(path)
            print(f"[SERVER] Deleted image after serving: {path}")
        except Exception as e:
            print(f"[SERVER] Error deleting image: {e}")
        return response
    return send_from_directory(IMAGE_DIR, filename)

def start_flask():
    app.run(host="0.0.0.0", port=PORT, debug=False, use_reloader=False)

class LoveByteServerApp:
    def __init__(self, root):
        self.root = root
        self.root.title("LoveByte Server (Port 6969)")
        self.device_list = tk.Listbox(root, width=28, height=10)
        self.device_list.grid(row=0, column=0, rowspan=13, padx=6, pady=6, sticky="ns")
        ttk.Label(root, text="Message:").grid(row=0, column=1, sticky="w")
        self.msg_entry = tk.Entry(root, width=48)
        self.msg_entry.grid(row=1, column=1, sticky="we", padx=2)

        # --- Send Message Button (RESTORED) ---
        self.send_msg_btn = tk.Button(root, text="Send Message", command=self.send_message, width=13)
        self.send_msg_btn.grid(row=2, column=1, sticky="e", padx=2, pady=(0, 6))

        # Color picker
        ttk.Label(root, text="LED Color:").grid(row=3, column=1, sticky="w")
        self.color_val = tk.StringVar(value="")
        self.color_btn = tk.Button(root, text="Pick Color", command=self.choose_color, width=10)
        self.color_btn.grid(row=4, column=1, sticky="w", padx=2)
        self.clear_led_btn = tk.Button(root, text="Clear LED Color", command=self.clear_led_color, width=13)
        self.clear_led_btn.grid(row=4, column=1, sticky="e", padx=2)

        # Heartbeat
        self.heartbeat_var = tk.BooleanVar()
        self.heartbeat_chk = ttk.Checkbutton(root, text="Heartbeat", variable=self.heartbeat_var)
        self.heartbeat_chk.grid(row=5, column=1, sticky="w", padx=2)

        # Heartbeat color picker
        ttk.Label(root, text="Heartbeat Color:").grid(row=6, column=1, sticky="w")
        self.hb_color_val = tk.StringVar(value="")
        self.hb_color_btn = tk.Button(root, text="Pick HB Color", command=self.choose_hb_color, width=13)
        self.hb_color_btn.grid(row=7, column=1, sticky="w", padx=2)
        self.clear_hb_btn = tk.Button(root, text="Clear HB Color", command=self.clear_hb_color, width=13)
        self.clear_hb_btn.grid(row=7, column=1, sticky="e", padx=2)

        # Heartbeat pulses (default: 3)
        ttk.Label(root, text="Heartbeat Pulses:").grid(row=8, column=1, sticky="w")
        self.heartbeat_pulses_entry = tk.Entry(root, width=12)
        self.heartbeat_pulses_entry.grid(row=8, column=1, sticky="e", padx=40)
        self.heartbeat_pulses_entry.insert(0, "3")

        # --- Image send controls ---
        ttk.Label(root, text="Send Image/GIF:").grid(row=9, column=1, sticky="w")
        self.image_path = tk.StringVar()
        self.image_entry = tk.Entry(root, textvariable=self.image_path, width=34)
        self.image_entry.grid(row=10, column=1, sticky="w", padx=2)
        self.browse_btn = tk.Button(root, text="Browse", command=self.browse_image, width=8)
        self.browse_btn.grid(row=10, column=1, sticky="e", padx=2)
        self.send_image_btn = tk.Button(root, text="Send Image", command=self.send_image, width=13)
        self.send_image_btn.grid(row=11, column=1, sticky="e", pady=3)

        self.log_box = tk.Text(root, width=48, height=10, state='disabled')
        self.log_box.grid(row=12, column=1, sticky="we", padx=2, pady=2)
        root.grid_columnconfigure(1, weight=1)
        self.refresh_devices()
        self.poll_gui_queue()

    def choose_color(self):
        color = colorchooser.askcolor(title="Pick LED Color")
        if color and color[1]:
            hexcolor = color[1].lstrip('#').upper()
            self.color_val.set(hexcolor)
            self.color_btn.config(text="#" + hexcolor)
        else:
            self.color_val.set("")
            self.color_btn.config(text="Pick Color")

    def clear_led_color(self):
        self.color_val.set("")
        self.color_btn.config(text="Pick Color")

    def choose_hb_color(self):
        color = colorchooser.askcolor(title="Pick Heartbeat Color")
        if color and color[1]:
            hexcolor = color[1].lstrip('#').upper()
            self.hb_color_val.set(hexcolor)
            self.hb_color_btn.config(text="#" + hexcolor)
        else:
            self.hb_color_val.set("")
            self.hb_color_btn.config(text="Pick HB Color")

    def clear_hb_color(self):
        self.hb_color_val.set("")
        self.hb_color_btn.config(text="Pick HB Color")

    def refresh_devices(self):
        sel = self.device_list.curselection()
        selected_id = None
        if sel:
            device_label = self.device_list.get(sel[0])
            selected_id = device_label.split(' ')[0]

        self.device_list.delete(0, tk.END)
        now = time.time()
        ids = []
        for dev_id, info in devices.items():
            status = "Online" if now - info['last_seen'] < 30 else "Offline"
            label = f"{dev_id} [{status}]"
            self.device_list.insert(tk.END, label)
            ids.append(dev_id)

        if selected_id:
            for i, dev_id in enumerate(ids):
                if dev_id == selected_id:
                    self.device_list.select_set(i)
                    break

        self.root.after(2000, self.refresh_devices)

    def send_message(self):
        sel = self.device_list.curselection()
        if not sel:
            messagebox.showerror("Select device", "Select a device from the list!")
            return
        device_label = self.device_list.get(sel[0])
        device_id = device_label.split(' ')[0]
        msg = self.msg_entry.get().strip()
        if not msg:
            messagebox.showerror("Empty", "Message text required!")
            return

        ledcolor = self.color_val.get().lstrip("#").upper() if self.color_val.get() else ""
        use_led = bool(ledcolor)
        heartbeat = bool(self.heartbeat_var.get())
        hbcolor = self.hb_color_val.get().lstrip("#").upper() if self.hb_color_val.get() else ""
        use_hbcolor = bool(hbcolor)
        try:
            pulses = int(self.heartbeat_pulses_entry.get())
        except Exception:
            pulses = 3

        if heartbeat and (not hbcolor or pulses <= 0):
            messagebox.showerror("Heartbeat Color Required", "You must pick a heartbeat color and pulses > 0 to enable heartbeat!")
            return

        payload = {
            'recipient': device_id,
            'text': msg,
            'sender': 'LoveByte server',
            'time': time.strftime("%Y-%m-%d %H:%M:%S"),
            'weather': '',
            'city': 'Modesto',
            'country': 'US',
            'tempF': 0,
            'ledColor': ledcolor,
            'useLedColor': use_led,
            'useHeartbeat': heartbeat,
            'heartbeatColor': hbcolor,
            'heartbeatPulses': pulses
        }
        print(f"[SERVER] Outbound: ledColor='{ledcolor}' heartbeatColor='{hbcolor}'")
        devices[device_id]['messages'].append(payload)
        self.log(f"Sent to {device_id}: {msg} (Color: {ledcolor or 'None'}, Heartbeat: {heartbeat}, Pulses: {pulses}, HB Color: {hbcolor or 'None'})")
        self.msg_entry.delete(0, tk.END)

    def browse_image(self):
        path = filedialog.askopenfilename(
            title="Select image or GIF",
            filetypes=[("Image Files", "*.jpg *.jpeg *.png *.gif")]
        )
        if path:
            self.image_path.set(path)

    def send_image(self):
        sel = self.device_list.curselection()
        if not sel:
            messagebox.showerror("Select device", "Select a device from the list!")
            return
        device_label = self.device_list.get(sel[0])
        device_id = device_label.split(' ')[0]
        imgfile = self.image_path.get()
        if not imgfile or not os.path.isfile(imgfile):
            messagebox.showerror("No Image", "Choose a valid image file first.")
            return

        url = f"http://127.0.0.1:{PORT}/api/upload_image"
        try:
            with open(imgfile, "rb") as f:
                files = {'file': (os.path.basename(imgfile), f, 'multipart/form-data')}
                data = {'recipient': device_id}
                resp = requests.post(url, files=files, data=data)
                if resp.ok:
                    self.log(f"Image sent to {device_id}: {os.path.basename(imgfile)}")
                    messagebox.showinfo("Upload Success", f"Image sent!\n{resp.json().get('file')}")
                else:
                    self.log(f"Failed to send image: {resp.text}")
                    messagebox.showerror("Upload Failed", resp.text)
        except Exception as e:
            self.log(f"Error sending image: {e}")
            messagebox.showerror("Error", str(e))

    def log(self, txt):
        self.log_box['state'] = 'normal'
        self.log_box.insert(tk.END, txt + '\n')
        self.log_box['state'] = 'disabled'
        self.log_box.see(tk.END)

    def poll_gui_queue(self):
        while not message_queue.empty():
            evt = message_queue.get()
            if evt['action'] == 'log':
                self.log(evt['msg'])
        self.root.after(500, self.poll_gui_queue)

def main():
    flask_thread = threading.Thread(target=start_flask, daemon=True)
    flask_thread.start()
    root = tk.Tk()
    app = LoveByteServerApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
