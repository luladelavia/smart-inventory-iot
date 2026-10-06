# Smart Inventory Bin (Rak Pintar IoT)

Project IoT monitoring stok barang pada rak secara real-time.

## Komponen & Fitur
- **Mikrokontroler:** ESP32 (disimulasikan via Wokwi)
- **Sensor:** Ultrasonic (mengukur tinggi/stok barang) & RTC DS1307
- **Protokol:** MQTT (Broker: broker.hivemq.com)
- **Topic:** `gudang/stok/rak1`
- **Dashboard:** Web HTML + Paho MQTT WebSocket

## Cara Menjalankan
1. Jalankan simulasi di Wokwi.
2. Buka file `index.html` di browser untuk melihat data real-time.