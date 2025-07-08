#!/usr/bin/env python
#########################################################################
# File Name: opencma.py
# Author: LiHongjin
# mail: 872648180@qq.com
# Created Time: Tue 08 Jul 2025 07:37:45 PM CST
#########################################################################

import cv2

cma_avlb = -1

for i in range(2):
    cap = cv2.VideoCapture(i)
    if cap.isOpened():
        ret, frame = cap.read()
        if ret:
            print(f"✅ Camera {i} Available")
            cma_avlb = i
        else:
            print(f"⚠️ Camera {i} is open but cannot read image")
        cap.release()
    else:
        print(f"❌ Camera {i} cannot be opened")

if cma_avlb < 0:
    exit(0)

cap = cv2.VideoCapture(cma_avlb)  # 0 表示默认摄像头 /dev/video0

if not cap.isOpened():
    print("Cannot open camera")
    exit()

while True:
    ret, frame = cap.read()
    if not ret:
        print("Can't receive frame. Exiting ...")
        break

    cv2.imshow('Camera', frame)

    if cv2.waitKey(1) == ord('q'):
        break

cap.release()
cv2.destroyAllWindows()

