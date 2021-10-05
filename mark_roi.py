import cv2
import json
import argparse

# variables
img = None
ix = -1
iy = -1
drawing = False
fx = -1
fy = -1
cx1 = -1
cy1 = -1
cx2 = -1
cy2 = -1
img_path = None
img_size = (768, 576)

def draw_reactangle_with_drag(event, x, y, flags, param):
    global ix, iy, fx, fy, drawing, img, img_path, cx1, cy1, cx2, cy2
    if event == cv2.EVENT_LBUTTONDOWN:
        drawing = True
        ix = x
        iy = y

    elif event == cv2.EVENT_MOUSEMOVE:
        if drawing == True:
            img2 = cv2.imread(img_path)
            img2 = cv2.resize(img2, img_size)
            cv2.rectangle(img2, pt1=(ix,iy), pt2=(x, y),color=(0,0,255),thickness=2)
            img = img2

    elif event == cv2.EVENT_LBUTTONUP:
        drawing = False
        img2 = cv2.imread(img_path)
        img2 = cv2.resize(img2, img_size)
        fx = x
        fy = y
        area = abs((fx-ix)*(fy-iy))
        print("area: ", area)
        if area < 200:
            print("Region of interest too small!\nSelect new ROI")
            cv2.rectangle(img2, pt1=(cx1,cy1), pt2=(cx2, cy2),color=(0,255,0),thickness=2)
            return
        cv2.rectangle(img2, pt1=(ix,iy), pt2=(x, y),color=(0,255,0),thickness=2)
        cv2.putText(img2, f'Top left point {ix, iy}', (40,30), cv2.FONT_HERSHEY_DUPLEX, 0.8, (255,0,0), 2)
        cv2.putText(img2, f'Bottom Right point {fx, fy}', (40,60), cv2.FONT_HERSHEY_DUPLEX, 0.8, (255,0,0), 2)
        img = img2
        cx1 = ix
        cy1 = iy
        cx2 = fx
        cy2 = fy

def main(args):
    global img, img_path
    print("-------------------------------------")
    print("\tInstructions")
    print("Press s to save the cordinates of ROI")
    print("Press ESC to exit")
    print("-------------------------------------")
    img_path = args["path"]
    img = cv2.imread(img_path)
    img = cv2.resize(img, img_size)
    cv2.namedWindow(winname= "Title of Popup Window")
    cv2.setMouseCallback("Title of Popup Window", draw_reactangle_with_drag)

    while True:
        cv2.imshow("Title of Popup Window", img)
        if cv2.waitKey(10) == 27:
            break
        elif cv2.waitKey(10) == ord('s'):
            config = {'ix': cx1, 'iy': cy1, 'fx':cx2, 'fy':cy2}
            print("Cordinates: ", config)
            with open('config.json', 'w') as f:
                json.dump(config, f)
            print("Saved ROI coordinates in config.json\n")
    cv2.destroyAllWindows()

def argsParser():
    arg_parse = argparse.ArgumentParser()
    arg_parse.add_argument("-p", "--path", required=True,type=str, help="path to image file")
    args = vars(arg_parse.parse_args())
    return args

if __name__ == "__main__":
    args = argsParser()
    main(args)
