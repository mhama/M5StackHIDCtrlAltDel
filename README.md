# About 概要
You can send CTRL+ALT+DEL to your PC with M5Stack! M5Stack behaves like BT keyboard via BLE HID.\
M5StackでCTRL+ALT+DELをPCに送れます！M5StackがBLE HIDデバイスつまりBTキーボードになり、PCやスマホにキーを送信できます。

<img src="https://user-images.githubusercontent.com/618417/38174769-6b217e7c-360d-11e8-9a94-2b2ac8410b8e.jpg" width="400px">

# Build ビルド
Follow instructions on the below link to setup Arduino for M5Stack. \
以下のリンクに従ってM5StackをArduino向けにセットアップしてください。

https://github.com/m5stack/M5Stack

Clone this project and compile M5StackHIDCtrlAltDel.ino and upload to M5Stack. \
このプロジェクトをCloneして、M5StackHIDCtrlAltDel.ino をコンパイルし、M5Stackにアップロードしてください。

# Usage 使用方法
It behaves like BT Keyboard. You need to pair the device with your PC/Smartphone. M5Stack is shown as "M5StackHID".
Bluetoothキーボードのように動作します。PCやスマホ上でBluetoothデバイスを追加してください。M5StackHIDという名前で表示されます。

When connected, you can send key code below when you push buttons of M5Stack. \
COnnectedと表示されていれば、ボタンを押すと以下のキーコードを送信します。

* A button: Hello from M5Stack!
* B button: CTRL+ALT+DEL :-)

# Restriction 制限事項
Currently, it does not send keys correctly after resetting M5Stack. Please remove the BLE device "M5StackHID" from PC/SmartPhone and add the device again. \
M5StackをリセットするとConnected状態でもキーを送信できないようです。その場合、PC/スマホ側でBTデバイス"M5StackHID"を削除してから、追加しなおしてください。

Resolution of this problem is welcome !!! \
解決方法募集中！！！

Also, please beware that it sometimes make my laptop's trackpad and keyboard sort of unresponsive... \
あと、たまにトラックパッドとキーボードがうまく動作しなくなることがあるので注意！
