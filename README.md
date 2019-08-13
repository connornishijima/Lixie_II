# Lixie II for Arduino

This library is a work-in-progress, but almost all of the functionality from the original Lixie 1/Edgelit libraries is here, and will have software examples within the next day or so! For now, you can [refer to the original library documentation](https://github.com/connornishijima/Lixie-arduino) for its usage is almost identical. Just change any ocurrance of "Lixie" in your code with "Lixie_II".

# WARNING

## UNLIKE THE ORIGINAL LIXIE LIBRARY, THIS ONE RELIES ON THE ESPRESSIF ARCHITECTURE. (ESP8266 / ESP32 - NOT AVR)

This decision was made to allow for non-blocking animation code, as the Ticker library or ESP32's second core are great for this. The vast majority of Lixie 1 users were using Espressif controllers anyways for a cheap gateway for the Lixies to fetch internet time, stocks, analytics, etc.
