// $Id$

#include "V9990.hh"
#include "V9990SDLRasterizer.hh"
#include <SDL.h>

using std::string;

namespace openmsx {

// Force template instantiation.
template class V9990SDLRasterizer<Uint16, Renderer::ZOOM_256>;
template class V9990SDLRasterizer<Uint16, Renderer::ZOOM_REAL>;
template class V9990SDLRasterizer<Uint32, Renderer::ZOOM_256>;
template class V9990SDLRasterizer<Uint32, Renderer::ZOOM_REAL>;

template <class Pixel, Renderer::Zoom zoom>
V9990SDLRasterizer<Pixel, zoom>::V9990SDLRasterizer(
	V9990* vdp_, SDL_Surface* screen_)
	: vdp(vdp_)
	, screen(screen_)
	, bitmapConverter(vdp->getVRAM(), *screen->format,
	                  palette64, palette256, palette32768)
{
	PRT_DEBUG("V9990SDLRasterizer::V9990SDLRasterizer");

	static const int width = (zoom == Renderer::ZOOM_256)
	                       ? SCREEN_WIDTH
	                       : SCREEN_WIDTH * 2;
	static const int height = (zoom == Renderer::ZOOM_256)
	                        ? SCREEN_HEIGHT
	                        : SCREEN_HEIGHT * 2;
	vram = vdp->getVRAM();

	/* Create Work screen */
	SDL_PixelFormat* format = screen->format;
	workScreen = SDL_CreateRGBSurface(
		SDL_SWSURFACE, width, height, format->BitsPerPixel,
		format->Rmask, format->Gmask, format->Bmask, format->Amask);

	/* Fill palettes */
	precalcPalettes();
}

template <class Pixel, Renderer::Zoom zoom>
V9990SDLRasterizer<Pixel, zoom>::~V9990SDLRasterizer()
{
	PRT_DEBUG("V9990SDLRasterizer::~V9990SDLRasterizer");

	if (workScreen) SDL_FreeSurface(workScreen);
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::paint()
{
	PRT_DEBUG("V9990SDLRasterizer::paint()");

	// Simple scaler for SDLHi
	if (zoom == Renderer::ZOOM_REAL) {
		SDL_Rect srcLine;
		SDL_Rect dstLine;
		srcLine.x = 0;
		srcLine.w = SCREEN_WIDTH * 2;
		srcLine.y = 0;
		srcLine.h = 1;

		dstLine.x = 0;
		dstLine.w = SCREEN_WIDTH * 2;
		dstLine.y = 1;
		dstLine.h = 1;
		
		for (int y = 0; y < SCREEN_HEIGHT; ++y) {
			SDL_BlitSurface(workScreen, &srcLine, workScreen, &dstLine);
			dstLine.y += 2;
			srcLine.y += 2;
		}
	} 
	SDL_BlitSurface(workScreen, NULL, screen, NULL);
}

template <class Pixel, Renderer::Zoom zoom>
const string& V9990SDLRasterizer<Pixel, zoom>::getName()
{
	static const string NAME = "V9990SDLRasterizer";
	return NAME;
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::reset()
{
	PRT_DEBUG("V9990SDLRasterizer::reset()");

	setBackgroundColor(vdp->getBackDropColor());
	setDisplayMode(vdp->getDisplayMode());
	setColorMode(vdp->getColorMode());
	imageWidth = vdp->getImageWidth();
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::frameStart(
	const V9990DisplayPeriod *horTiming,
	const V9990DisplayPeriod *verTiming)
{
	PRT_DEBUG("V9990SDLRasterizer::frameStart()");

	/* Center image on the window.
	 *
	 * In SDLLo, one window pixel represents 8 UC clockticks, so the
	 * window = 320 * 8 UC ticks wide. In SDLHi, one pixel is 4 clock-
	 * ticks and the window 640 pixels wide -- same amount of UC ticks.
	 */
	colZero  = horTiming->border1 + 
	           (horTiming->display - SCREEN_WIDTH * 8) / 2;

	/* 240 display lines can be shown. In SDLHi, we can do interlace,
	 * but still 240 lines per frame.
	 */
	lineZero = verTiming->border1 +
	           (verTiming->display - SCREEN_HEIGHT) / 2;
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::frameEnd()
{
	PRT_DEBUG("V9990SDLRasterizer::frameEnd()");
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::setDisplayMode(V9990DisplayMode mode)
{
	PRT_DEBUG("V9990SDLRasterizer::setDisplayMode(" << std::dec <<
	          (int) mode << ")");

	displayMode = mode;
	bitmapConverter.setDisplayMode(mode);
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::setColorMode(V9990ColorMode mode)
{
	PRT_DEBUG("V9990SDLRasterizer::setColorMode(" << std::dec <<
	          (int) mode << ")");

	colorMode = mode;
	bitmapConverter.setColorMode(mode);
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::setImageWidth(int width)
{
	// sync?
	imageWidth = width;
}

#define translateX(x)  ((x)/((zoom == Renderer::ZOOM_256)?8:4))
#define translateY(y)  (y*((zoom == Renderer::ZOOM_256)?1:2))

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::drawBorder(
	int fromX, int fromY, int toX, int toY)
{
	PRT_DEBUG("V9990SDLRasterizer::drawBorder(" << std::dec <<
	          fromX << "," << fromY << "," << toX << "," << toY << ")");

	static int const screenW = SCREEN_WIDTH * 8;
	static int const screenH = SCREEN_HEIGHT;

	// From vdp coordinates (UC ticks/lines) to screen coordinates
	fromX -= colZero;
	fromY -= lineZero;
	toX   -= colZero;
	toY   -= lineZero;

	// Clip or expand to screen edges
	if ((fromX <= -colZero) || (fromX < 0)) {
		fromX = 0;
	}
	if ((fromY <= -lineZero) || (fromY < 0)) {
		fromY = 0;
	}
	if ((toX >= screenW + colZero) || (toX > screenW)) {
		toX = screenW;
	}
	if ((toY >= screenH + lineZero) || (toY > screenH)) {
		toY = screenH;
	}

	int width = toX - fromX;
	int height = toY - fromY;

	if ((width > 0) && (height > 0)) {
		SDL_Rect rect;

		rect.x = translateX(fromX); rect.y = translateY(fromY);
		rect.w = translateX(width); rect.h = translateY(height);

		SDL_FillRect(workScreen, &rect, bgColor);
	}
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::drawDisplay(
	int fromX, int fromY,
	int displayX, int displayY, int displayWidth, int displayHeight )
{
	PRT_DEBUG("V9990SDLRasterizer::drawDisplay(" << std::dec <<
	          fromX << "," << fromY << "," <<
	          displayX << "," << displayY << "," <<
	          displayWidth << "," << displayHeight << ")");

	static int const screenW = SCREEN_WIDTH * 8;
	static int const screenH = SCREEN_HEIGHT;

	if ((displayWidth > 0) && (displayHeight > 0)) {
		
		// from VDP coordinates to screen coordinates
		fromX -= colZero;
		fromY -= lineZero;

		// Clip to screen
		if (fromX < 0) {
			displayX -= fromX;
			displayWidth += fromX;
			fromX = 0;
		}
		if ((fromX + displayWidth) > screenW) {
			displayWidth = screenW - fromX;
		}
		if (fromY < 0) {
			displayY -= fromY;
			displayHeight += fromY;
			fromY = 0;
		}
		if ((fromY + displayHeight) > screenH) {
			displayHeight = screenH - fromY;
		}
		
		SDL_Rect rect;
		rect.x = translateX(fromX);
		rect.y = translateY(fromY);
		rect.w = translateX(displayWidth);
		rect.h = translateY(displayHeight);

		if (displayMode == P1) {
			// not yet implemented
			SDL_FillRect(workScreen, &rect, bgColor);
		} else if(displayMode == P2) {
			// not implemented yet
			
		} else {
			SDL_FillRect(workScreen, &rect, (Pixel) 0);
			int vramStep;
			Pixel* pixelPtr = (Pixel*)(
		                   (byte*)workScreen->pixels +
		                  + translateY(fromY) * workScreen->pitch
		                  + translateX(fromX) * sizeof(Pixel));

			displayX = V9990::UCtoX(displayX, displayMode);
			displayWidth = V9990::UCtoX(displayWidth, displayMode);
			uint address = vdp->XYtoVRAM(&displayX, displayY, colorMode);
			switch (colorMode) {
				case BP2:
					vramStep = imageWidth / 4;
					break;
				case BP4:
				case PP:
					vramStep = imageWidth / 2;
					break;
				case BD16:
					vramStep = imageWidth * 2;
					break;
				default:
					vramStep = imageWidth;
			}
				
			while (displayHeight--) {
				bitmapConverter.convertLine(pixelPtr, address, displayWidth);
				
				// next line
				address += vramStep;
				pixelPtr += workScreen->w * translateY(1);
			}
		}
	}
}


template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::precalcPalettes()
{
	PRT_DEBUG("V9990SDLRasterizer::precalcPalettes()");

	// the 32768 color palette
	for (int g = 0; g < 32; ++g) {
		for (int r = 0; r < 32; ++r)
			for (int b = 0; b < 32; ++b) 
				palette32768[(g << 10) + (r << 5) + b] =
					SDL_MapRGB(screen->format,
					           (int)(r * (255.0 / 31.0)),
					           (int)(g * (255.0 / 31.0)),
					           (int)(b * (255.0 / 31.0)));
	}
	
	// the 256 color palette
	int mapRG[8] = { 0, 4, 9, 13, 18, 22, 27, 31 };
	int mapB [4] = { 0, 11, 21, 31 };
	for (int g = 0; g < 8; ++g)
		for (int r = 0; r < 8; ++r)
			for (int b = 0; b < 4; ++b)
				palette256[(g << 5) + (r << 2) + b] = 
					palette32768[(mapRG[g] << 10) +
					             (mapRG[r] <<  5) +
					              mapB [b]];
	
	// TODO Get 64 color palette from VDP
}

template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::setPalette(int index,
                                                 byte r, byte g, byte b)
{
	palette64[index & 63] = palette32768[((g & 31) << 10) + 
	                                     ((r & 31) <<  5) +
	                                      (b & 31)]; 
}


template <class Pixel, Renderer::Zoom zoom>
void V9990SDLRasterizer<Pixel, zoom>::setBackgroundColor(int index)
{
	bgColor = palette64[index & 63];
}

} // namespace openmsx

