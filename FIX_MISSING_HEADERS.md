# FIX: Install Missing Headers in Arduino IDE

## **The 3 Required Libraries (MUST INSTALL):**

Your code needs exactly 3 libraries. Follow these steps:

---

## **STEP 1: Install Adafruit MPU6050**

1. Open **Arduino IDE 2.x**
2. Click **Sketch** → **Include Library** → **Manage Libraries**
   - Or press `Ctrl+Shift+I`
3. Search box: type `Adafruit MPU6050`
4. Click the result by **Adafruit**
5. Click **Install**
6. Wait for it to finish

---

## **STEP 2: Install Adafruit Sensor**

1. In the same **Manage Libraries** window
2. Search: `Adafruit Sensor`
3. Click result by **Adafruit**
4. Click **Install**
5. Wait for it to finish

---

## **STEP 3: Verify Headers Are Available**

After installing, you should see these in Arduino IDE:

```cpp
#include <Wire.h>           // Built-in (no install needed)
#include <Adafruit_MPU6050.h>   // ✅ Just installed
#include <Adafruit_Sensor.h>    // ✅ Just installed
```

---

## **STEP 4: Select Your Board**

Before uploading:

1. **Tools** → **Board** → **Boards Manager**
2. Search: `Arduino Portenta`
3. Install: **Arduino Mbed OS Portenta Boards**
4. Then go **Tools** → **Board** → Select your board (e.g., Arduino Portenta H7)

---

## **STEP 5: Compile Test**

1. Open `arduino_microros_robot.ino`
2. Click **Verify** (✓ checkmark button) to compile
3. Should now show: ✅ **Compilation successful**

If you see errors, check:
- All 3 libraries installed? (Manage Libraries shows them with version numbers)
- Board selected? (Tools → Board shows your board name)
- Are there red squiggly lines in the code? (Restart Arduino IDE)

---

## **If You See This Error:**

```
fatal error: Adafruit_MPU6050.h: No such file or directory
```

**Solution:**
1. Go to **Sketch** → **Include Library** → **Manage Libraries**
2. Search `Adafruit MPU6050`
3. Make sure it's **installed** (click **Install** if not)
4. Restart Arduino IDE
5. Try compiling again

---

## **Quick Checklist:**

- [ ] Adafruit MPU6050 library installed
- [ ] Adafruit Sensor library installed
- [ ] Arduino Mbed OS Portenta Boards installed
- [ ] Board selected (Tools → Board)
- [ ] Port selected (Tools → Port → COM XX)
- [ ] Code clicked **Verify** ✓ with no errors

Once all checkboxes are done, click **Upload** (→) to flash to your Arduino AI!
