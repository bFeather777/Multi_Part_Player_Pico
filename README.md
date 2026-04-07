# 25-04-06-Mon
1.重新改用C++來撰寫此程式
2.焊好兩顆pico，但是觀察到其訊號不同步

# 25-04-07-Tue
1.硬體通訊與準位衝突:於RST腳位量到3.7V，認為應為Arduino Nano的5V與RC522的3.3V之間的高電位不匹配。
2.需確認Arduino Nano的輸出訊號電位是否造成RC522運作不正常。
3.RC522天線的啟動電源較大，應無法由Arduino Nano供應，因Arduino Nano的輸出電流較弱=>Pico也曾發生
  類似的狀況。
4.試著減少線長，以避免雜訊。
5.重新撰寫另一首曲子的兩聲部，由人耳聽起來應為同步，應為昨天寫的曲子兩聲部有錯誤，才會有訊號不同步的判斷。

# Hardware
| Nano Pin | Function  | 
------------------------
|      D9  |       RST | 
|      D10 | SS (Sync) | 
|      D11 |      MOSI |
|      D12 |      MISO |     
|      D13 |      SCK  | 
|      GND |    Ground | 