#include "camDemo.h"

void mouse_event(int evt, int x, int y, int, void* param)
{
	MouseParams* mp = (MouseParams*)param;
	mp->evt = evt; //Mouse-Event
	mp->mouse_pos.x = x; //Mouse x-Position
	mp->mouse_pos.y = y; //Mouse y-Position
}

bool click_left(MouseParams mp, char* folder)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		{
			char path[512];
			sprintf_s(path, "%s/black_hole.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			return true;
		}
	}
	return false;
}

bool click_in_rect(MouseParams mp, Rect rect, char* folder)
{
	if (mp.evt == EVENT_LBUTTONDOWN)
	{
		if (mp.mouse_pos.x >= rect.x &&
			mp.mouse_pos.y >= rect.y &&
			mp.mouse_pos.x <= rect.x + rect.width &&
			mp.mouse_pos.y <= rect.y + rect.height)
		{
			char path[512];
			sprintf_s(path, "%s/black_hole.wav", folder);
			PlaySoundA(path, NULL, SND_ASYNC);
			return true;
		}
	}
	return false;
}

bool mouse_in_rect(MouseParams mp, Rect rect)
{
	if (mp.evt == EVENT_MOUSEMOVE)
	{
		if (mp.mouse_pos.x >= rect.x &&
			mp.mouse_pos.y >= rect.y &&
			mp.mouse_pos.x <= rect.x + rect.width &&
			mp.mouse_pos.y <= rect.y + rect.height)
		{
			return true;
		}
	}
	return false;
}

int main(int, char**)
{
	const char* folder1 = "Resources";
	const char* folder2 = "../Resources";
	char folder[15], path[512];

	MouseParams mp;
	Scalar colour;
	Mat	cam_img;
	Mat cam_img_grey;
	Mat strElement;
	Mat3b rgb_scale;
	char* windowGameOutput = "Black Hole Effect - BiVa1"; // Name of window
	unsigned int width, height, channels, stride;	// Werte des angezeigten Bildes
	int
		key = 0,	// Tastatureingabe
		frames = 0, //frames z‰hlen f¸r FPS-Anzeige
		fps = 0;	//frames pro Sekunde
	int camNum = 2;
	bool fullscreen_flag = false; //Ist fullscreen aktivert oder nicht?
	bool median_flag = false;
	bool flip_flag = true;
	float scalingFactor = 1.0f;
	float radiusMult = 1.0;
	float effectSpeed = 2.0f;
	int decrement_counter = 0;
	int increment_counter = 0;
	int initialCounter;
	bool animation_flag = false;
	bool start_animation = false;
	bool frozen = false;
	DemoState state; //Aktueller Zustand des Spiels
	#if defined _DEBUG || defined LOGGING
	FILE* log = NULL;
	log = fopen("log_debug.txt", "wt");
	#endif

	clock_t start_time, finish_time;

	VideoCapture cap(CAP_DSHOW);
	do
	{
		camNum--; /* try next camera */
		cap.open(camNum);
	} while (camNum > 0 && !cap.isOpened()); /* solange noch andere Kameras verf¸gbar sein kˆnnten */

	if (!cap.isOpened())	// ist die Kamera nicht aktiv?
	{
		AllocConsole();
		printf("Keine Kamera gefunden!\n");
		printf("Zum Beenden 'Enter' druecken\n");
		#if defined _DEBUG || defined LOGGING
		fprintf(log, "Keine Kamera gefunden!\n");
		fclose(log);
		#endif
		while (getchar() == NULL);
		return -1;
	}
	else
	{
		/* some infos on console	*/
		printf("==> Program Control <==\n");
		printf("==                   ==\n");
		printf("* Start Screen\n");
		printf(" - 'ESC' Stop the program \n");
		printf(" - 'm'   Increase speed of animation\n");
		printf(" - 'l'   Decrease speed of animation\n");
		printf(" - 'f'   Freeze animation\n");
		printf(" - 's'   Stop animation\n");
	}
	{
		HWND console = GetConsoleWindow();
		RECT r;
		GetWindowRect(console, &r); //stores the console's current dimensions

		//MoveWindow(window_handle, x, y, width, height, redraw_window);
		//MoveWindow( console, r.left, r.top, 800, 600, TRUE);
		MoveWindow(console, 10, 0, 850, 800, TRUE);
	}

	#ifndef _DEBUG
	FreeConsole(); //Konsole ausschalten
	#endif

	// Set the resolution (16:9 to avoid black borders)
	int frameWidth = 768;
	int frameHeight = 450;
	cap.set(cv::CAP_PROP_FRAME_WIDTH, frameWidth);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, frameHeight);

	/* capture the image */
	cap >> cam_img;

	/* get format of camera image	*/
	width = cam_img.cols;
	height = cam_img.rows;
	channels = cam_img.channels();
	stride = width * channels;

	//Handle f¸r das Fenster vorbereiten
	namedWindow(windowGameOutput, WINDOW_NORMAL | CV_GUI_EXPANDED); //Erlauben des Maximierens
	resizeWindow(windowGameOutput, width, height); //Start Auflˆsung der Kamera
	HWND cvHwnd = (HWND)cvGetWindowHandle(windowGameOutput); //window-handle to detect window-states

	srand((unsigned)time(NULL));//seeds the random number generator

	/* find folder for ressources	*/
	{
		FILE* in = NULL;
		strcpy_s(folder, folder1);/* try first folder */
		sprintf_s(path, "%s/black_hole.wav", folder);
		in = fopen(path, "r");
		if (in == NULL)
		{
			strcpy_s(folder, folder2); /* try other folder */
			sprintf_s(path, "%s/black_hole.wav", folder);
			in = fopen(path, "r");
			if (in == NULL)
			{
				printf("Resources cannot be found\n");
				exit(99);
			}
		}
		fclose(in);
	}

	start_time = clock();


	/* structure element for dilation of binary image */
	//{
	//	int strElRadius = 3;
	//	int size = strElRadius * 2 + 1;
	//	strElement = Mat( size, size, CV_8UC1, Scalar::all( 0)); 
	//	//Einzeichnen des eigentlichen Strukturelements (weiﬂer Kreis)
	//	/* ( , Mittelpunkt, radius, weiﬂ, thickness=gef¸llt*/
	//	circle( strElement, Point( strElRadius, strElRadius), strElRadius, Scalar( 255), -1);
	//}

	state = START_SCREEN;

	// Setup zum Auswerten von Mausevents
	setMouseCallback(windowGameOutput, mouse_event, (void*)&mp);


	/*-------------------- main loop ---------------*/
	while (state != DEMO_STOP)
	{
		// ein Bild aus dem Video betrachten und in cam_img speichern
		if (cap.read(cam_img) == false)
		{ //falls nicht mˆglich: Fehlermeldung
			destroyWindow("camDemo"); //Ausgabefenster ausblenden

			AllocConsole(); //Konsole wieder einschalten
			printf("Verbindung zur Kamera verloren!\n");
			printf("Zum Beenden 'Enter' druecken\n");
			cap.release(); //Freigabe der Kamera
			#if defined _DEBUG || defined LOGGING
			fprintf(log, "Verbindung zur Kamera verloren!\n");
			fclose(log);
			#endif
			while (getchar() == NULL); //Warten auf Eingabe
			break; //Beende die Endlosschleife
		}

		if (flip_flag) flip(cam_img, cam_img, 1); // cam_img vertikal spiegeln
		// Strutz cvtColor( cam_img, rgb, CV_BGR2RGB); // Konvertierung BGR zu RGB

		//Runterskalierung des Bildes f¸r weniger Rechaufwand (Faktor 1/2)
		//resize(cam_img, rgb_scale, Size(), scale, scale);

		/* smoothing of images */
		if (median_flag) /* can be toggled with key 'm'*/
		{
			medianBlur(cam_img, cam_img, 7);
		}

		/* determination of frames per second	*/
		frames++;
		finish_time = clock();
		if ((double)(finish_time - start_time) / CLOCKS_PER_SEC >= 1.0)
		{
			fps = frames;
			frames = 0;
			start_time = clock();
		}

		#define FPS_OUTPUT
		#if defined FPS_OUTPUT || defined _DEBUG
		printf("FPS: %d\n", fps);
		//FPS-Ausgabe oben rechts
		char fps_char[3];
		sprintf(fps_char, "%d", fps);
		const string& fps_string = (string)fps_char;
		putText(cam_img, fps_string, Point(width - 40, 25), FONT_HERSHEY_SIMPLEX,
			0.5 /*fontScale*/, Scalar(0, 255, 255), 2);
		#endif

		/* input from keyboard */
		key = tolower(waitKey(1)); /* Strutz  convert to lower case */
		// Vollbildschirm ein- bzw. ausschalten
		if (key == 'f')
		{
			frozen = !frozen;
			if (frozen)
			{
				PlaySound(NULL, NULL, 0); // cancel sound
			}
			else
			{
				char path[512];
				sprintf_s(path, "%s/black_hole.wav", folder);
				PlaySoundA(path, NULL, SND_ASYNC);
			}
		}
		if (key == 'm') /* toggle flag */
		{
			effectSpeed += 0.5f;
		}
		else if (key == 'l')
		{
			if (effectSpeed > 0.5f)
			{
				effectSpeed -= 0.5f;
			}
			else
			{
				effectSpeed = 0.0f;
			}

		}
		else if (key == 's')
		{
			start_animation = false;
			PlaySound(NULL, NULL, 0); // cancel sound
		}

		if (state == START_SCREEN)
		{
			if (key == 27) // Abbruch mit ESC
			{
				state = DEMO_STOP; /* leave loop	*/
				continue;
			}
			else if (key == 'p')
			{
				/* show properties of camera	*/
				if (cap.set(CAP_PROP_SETTINGS, 0) != 1)
				{
					#if defined _DEBUG || defined LOGGING
					fprintf(log, "\nlocal webcam > Webcam Settings cannot be opened!\n");
					#endif
				}
			}
		}

		if (click_left(mp, folder))
		{
			start_animation = true;
			initialCounter = width / 2 + 50;
			decrement_counter = initialCounter;
			increment_counter = 0;
			scalingFactor = 1.0f;
		}

		if (start_animation)
		{
			if (!frozen)
			{
				float progress = (float)decrement_counter / initialCounter; // normalize (1.0 to 0.0)

				// Adjust speed for both increment and decrement based on progress
				float effect_speed = 1 + (1 - progress) * effectSpeed; // This is used for both increment and decrement

				// Decrement counter, based on effect speed
				decrement_counter -= (int)effect_speed;

				// Increment counter, using the same effect speed to maintain consistent rate
				increment_counter += (int)effect_speed + 2;

				// make sure counter does not go below 0
				if (decrement_counter < 0) decrement_counter = 0;

				scalingFactor += 0.025f; // increase the distortion
			}

			bool continueAnimation = createBlackHoleEffect(cam_img, mp.mouse_pos.x, mp.mouse_pos.y, decrement_counter, radiusMult, scalingFactor, 40, increment_counter);

			// end animation
			//if (continueAnimation == false)
			//{
			//	start_animation = false;
			//	PlaySound(NULL, NULL, 0); // cancel sound
			//}
			if (increment_counter > width * 1.5)
			{
				start_animation = false;
				PlaySound(NULL, NULL, 0); // cancel sound
			}
		}

		/********************************************************************************************/
		/* show window with live video	*/		//Le-Wi: Funktionalit‰ten zum Schlieﬂen (x-Button)
		if (!IsWindowVisible(cvHwnd))
		{
			break;
		}
		imshow(windowGameOutput, cam_img); //Ausgabefenster darstellen		
	}	// Ende der Endlos-Schleife

	//Freigabe aller Matrizen
	if (cap.isOpened()) cap.release(); //Freigabe der Kamera
	if (cam_img.data) cam_img.release();
	if (cam_img_grey.data) cam_img_grey.release();
	if (rgb_scale.data) rgb_scale.release();
	if (strElement.data) strElement.release();


	#if defined _DEBUG || defined LOGGING
	fclose(log);
	#endif
	//FreeConsole(); //Konsole ausschalten
	cvDestroyAllWindows();
	//_CrtDumpMemoryLeaks();
	exit(0);
}


//void createBlackHoleEffect(cv::Mat& inputImage, int centreX, int centreY, int radius, int radiusMult, float scalingFactor, int marginWidth) {
//	int rows = inputImage.rows;
//	int cols = inputImage.cols;
//
//	// Create an output image initialized to black
//	cv::Mat outputImage = cv::Mat::zeros(inputImage.size(), CV_8UC3);
//
//	for (int y = 0; y < rows; y++) {
//		for (int x = 0; x < cols; x++) {
//
//			// Calculate the distance of the current pixel from the black hole center
//			float distanceX = x - centreX;
//			float distanceY = y - centreY;
//			float distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);
//
//			if (distance < radius) {
//				// Compute the angle of the current pixel relative to the center
//				float angle = std::atan2(distanceY, distanceX);
//
//				// Introduce rotational distortion based on the distance
//				float rotationAmount = scalingFactor * (1.0f - distance / radius); // Decrease rotation with proximity
//				angle += rotationAmount;
//
//				// Compute scaled distance for the black hole effect
//				float scale = std::pow(distance / radius, 5);
//				float distortedDistance = distance + scale * radiusMult;
//
//				// Convert polar coordinates back to Cartesian coordinates
//				float sourceX = centreX + distortedDistance * std::cos(angle);
//				float sourceY = centreY + distortedDistance * std::sin(angle);
//
//				// Ensure the source position is within bounds
//				if (sourceX >= 0 && sourceX < cols && sourceY >= 0 && sourceY < rows) {
//					// Get pointers to the rows of the input and output images
//					uchar* outputPixel = outputImage.ptr<uchar>(y);
//					const uchar* inputPixel = inputImage.ptr<uchar>(static_cast<int>(sourceY));
//
//					// Compute the source and destination pixel positions
//					int sourceIdx = static_cast<int>(sourceX) * 3; // 3 channels (BGR)
//					int destIdx = x * 3;
//
//					// Copy the pixel values from input to output
//					outputPixel[destIdx + 0] = inputPixel[sourceIdx + 0]; // Blue
//					outputPixel[destIdx + 1] = inputPixel[sourceIdx + 1]; // Green
//					outputPixel[destIdx + 2] = inputPixel[sourceIdx + 2]; // Red
//				}
//			}
//			else if (distance < radius + marginWidth)
//			{
//				// Calculate gradient factor (0 at radius, 1 at radius + marginWidth)
//				float gradientFactor = (distance - radius) / marginWidth;
//
//				// Use a sigmoid-like easing function for smooth blending
//				float easedFactor = 1.0f / (1.0f + std::exp(-10 * (gradientFactor - 0.5)));
//
//				// Darken the outer edge towards black
//				float brightness = 1.0f - easedFactor; // Adjust brightness with easedFactor
//
//				// Interpolate color: dark edge to bright margin
//				uchar blue = static_cast<uchar>(brightness * 3);
//				uchar green = static_cast<uchar>(brightness * 107);
//				uchar red = static_cast<uchar>(brightness * 252);
//
//				uchar* pixelPtr = outputImage.ptr<uchar>(y); // Get pointer to the start of the row
//				pixelPtr[x * 3 + 0] = blue;  // Set the blue channel
//				pixelPtr[x * 3 + 1] = green; // Set the green channel
//				pixelPtr[x * 3 + 2] = red;   // Set the red channel
//			}
//		}
//	}
//
//	// Copy the result back to the input image
//	inputImage = outputImage.clone();
//}


bool createBlackHoleEffect(cv::Mat& inputImage, int centreX, int centreY, int radius, int radiusMult, float scalingFactor, int marginWidth, int counter) {
	int rows = inputImage.rows;
	int cols = inputImage.cols;

	// Create an output image initialized to black
	cv::Mat outputImage = cv::Mat::zeros(inputImage.size(), CV_8UC3);

	float innerBlackCircleRadius = counter / 14.0f;
	float marginRadius = innerBlackCircleRadius + marginWidth;
	float outerRadius = radius;

	//if (marginRadius > outerRadius)
	//{
	//	return false;
	//}

	for (int y = 0; y < rows; y++) {
		for (int x = 0; x < cols; x++) {

			// Calculate the distance of the current pixel from the black hole center
			float distanceX = x - centreX;
			float distanceY = y - centreY;
			float distance = std::sqrt(distanceX * distanceX + distanceY * distanceY);

			if (distance < innerBlackCircleRadius) {
				// Inside the black hole: set the pixel to black
				uchar* pixelPtr = outputImage.ptr<uchar>(y);
				pixelPtr[x * 3 + 0] = 0;  // Blue
				pixelPtr[x * 3 + 1] = 0;  // Green
				pixelPtr[x * 3 + 2] = 0;  // Red
			}
			else if (distance < marginRadius)
			{
				// Calculate gradient factor (0 at radius, 1 at radius + marginWidth)
				float gradientFactor = (distance - innerBlackCircleRadius) / marginWidth;

				// Use a sigmoid-like easing function for smooth blending
				float easedFactor = 1.0f / (1.0f + std::exp(-10 * (gradientFactor - 0.5)));

				// Darken the outer edge towards black
				float brightness = 1.0f - easedFactor; // Adjust brightness with easedFactor

				// Interpolate color: dark edge to bright margin
				uchar blue = static_cast<uchar>(brightness * 3);
				uchar green = static_cast<uchar>(brightness * 107);
				uchar red = static_cast<uchar>(brightness * 252);

				uchar* pixelPtr = outputImage.ptr<uchar>(y); // Get pointer to the start of the row
				pixelPtr[x * 3 + 0] = blue;  // Set the blue channel
				pixelPtr[x * 3 + 1] = green; // Set the green channel
				pixelPtr[x * 3 + 2] = red;   // Set the red channel
			}
			else if (distance < outerRadius) {
				// Apply rotational distortion for the outer region
				float angle = std::atan2(distanceY, distanceX);

				// Introduce rotational distortion based on the distance
				float normalizedDistance = (distance - marginRadius) / (outerRadius - marginRadius);
				float rotationAmount = scalingFactor * (1.0f - normalizedDistance); // Stronger near the black hole
				angle -= rotationAmount; // Pixels are "sucked in" by reducing the angle

				// Compute distorted distance (pull pixels inward)
				float scale = std::pow((1.0f - normalizedDistance), 5.0f); // Stronger inward pull near the black hole
				float distortedDistance = distance - scale * radiusMult;

				// Ensure distortedDistance does not fall into the black or margin areas
				if (distortedDistance < marginRadius) {
					distortedDistance = marginRadius;
				}

				// Convert polar coordinates back to Cartesian coordinates
				float sourceX = centreX + distortedDistance * std::cos(angle);
				float sourceY = centreY + distortedDistance * std::sin(angle);

				// Ensure the source position is within bounds
				if (sourceX >= 0 && sourceX < cols && sourceY >= 0 && sourceY < rows) {
					// Get pointers to the rows of the input and output images
					uchar* outputPixel = outputImage.ptr<uchar>(y);
					const uchar* inputPixel = inputImage.ptr<uchar>(static_cast<int>(sourceY));

					// Compute the source and destination pixel positions
					int sourceIdx = static_cast<int>(sourceX) * 3; // 3 channels (BGR)
					int destIdx = x * 3;

					// Copy the pixel values from input to output
					outputPixel[destIdx + 0] = inputPixel[sourceIdx + 0]; // Blue
					outputPixel[destIdx + 1] = inputPixel[sourceIdx + 1]; // Green
					outputPixel[destIdx + 2] = inputPixel[sourceIdx + 2]; // Red
				}
				// Else, leave the pixel black (default initialization of outputImage)
			}
		}
	}

	// Copy the result back to the input image
	inputImage = outputImage.clone();
	return true;
}






