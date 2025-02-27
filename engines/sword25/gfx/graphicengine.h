/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * This code is based on Broken Sword 2.5 engine
 *
 * Copyright (c) Malte Thiesen, Daniel Queteschiner and Michael Elsdoerfer
 *
 * Licensed under GNU GPL v2
 *
 */

/*
 * GraphicEngine
 * ----------------
 * This the graphics engine interface.
 *
 * Autor: Malte Thiesen
 */

#ifndef SWORD25_GRAPHICENGINE_H
#define SWORD25_GRAPHICENGINE_H

// Includes
#include "common/array.h"
#include "common/rect.h"
#include "common/ptr.h"
#include "common/str.h"
#include "graphics/surface.h"
#include "sword25/kernel/common.h"
#include "sword25/kernel/resservice.h"
#include "sword25/kernel/persistable.h"
#include "sword25/gfx/renderobjectptr.h"
#include "sword25/math/vertex.h"

namespace Sword25 {

class Kernel;
class Image;
class Panel;
class Screenshot;
class RenderObjectManager;

#define BS_ASHIFT 24
#define BS_RSHIFT 16
#define BS_GSHIFT 8
#define BS_BSHIFT 0

#define BS_AMASK 0xFF000000
#define BS_RMASK 0x00FF0000
#define BS_GMASK 0x0000FF00
#define BS_BMASK 0x000000FF

#define BS_RGBMASK (BS_RMASK | BS_GMASK | BS_BMASK)
#define BS_ARGBMASK (BS_AMASK | BS_RMASK | BS_GMASK | BS_BMASK)

#define BS_RGB(R,G,B)       (BS_AMASK | ((R) << BS_RSHIFT) | ((G) << BS_GSHIFT) | ((B) << BS_BSHIFT))
#define BS_ARGB(A,R,G,B)    (((A) << BS_ASHIFT) | ((R) << BS_RSHIFT) | ((G) << BS_GSHIFT) | ((B) << BS_BSHIFT))

/**
 * This is the graphics engine. Unlike the original code, this is not
 * an interface that needs to be subclassed, but rather already contains
 * all required functionality.
 */
class GraphicEngine : public ResourceService, public Persistable {
public:
	// Constructor
	// -----------
	GraphicEngine(Kernel *pKernel);
	~GraphicEngine() override;

	// Interface
	// ---------

	/**
	 * Initializes the graphics engine and sets the screen mode. Returns
	 * true if initialisation failed.
	 * @note This method should be called immediately after the
	 * initialisation of all services.
	 *
	 * @param Height            The height of the output buffer in pixels. The default value is 600
	 * @param BitDepth          The bit depth of the desired output buffer in bits. The default value is 16
	 * @param BackbufferCount   The number of back buffers to be created. The default value is 2
	 */
	bool init(int width = 800, int height = 600, int bitDepth = 16, int backbufferCount = 2);

	/**
	 * Begins rendering a new frame.
	 * Notes: This method must be called at the beginning of the main loop, before any rendering methods are used.
	 * Notes: Implementations of this method must call _UpdateLastFrameDuration()
	 * @param UpdateAll         Specifies whether the renderer should redraw everything on the next frame.
	 * This feature can be useful if the renderer with Dirty Rectangles works, but sometimes the client may
	*/
	bool startFrame(bool updateAll = false);

	/**
	 * Ends the rendering of a frame and draws it on the screen.
	 *
	 * This method must be at the end of the main loop. After this call, no further Render method may be called.
	 * This should only be called once for a given previous call to #StartFrame.
	*/
	bool endFrame();

	/**
	 * Creates a thumbnail with the dimensions of 200x125. This will not include the top and bottom of the screen..
	 * the interface boards the image as a 16th of it's original size.
	 * Notes: This method should only be called after a call to EndFrame(), and before the next call to StartFrame().
	 * The frame buffer must have a resolution of 800x600.
	 * @param Filename  The filename for the screenshot
	 */
	bool saveThumbnailScreenshot(const Common::String &filename);

	RenderObjectPtr<Panel> getMainPanel();

	/**
	 * Specifies the time (in microseconds) since the last frame has passed
	 */
	int getLastFrameDurationMicro() const {
		if (!_timerActive)
			return 0;
		return _lastFrameDuration;
	}

	/**
	 * Specifies the time (in microseconds) the previous frame took
	*/
	float getLastFrameDuration() const {
		if (!_timerActive)
			return 0;
		return static_cast<float>(_lastFrameDuration) / 1000000.0f;
	}

	void stopMainTimer() {
		_timerActive = false;
	}

	void resumeMainTimer() {
		_timerActive = true;
	}

	float getSecondaryFrameDuration() const {
		return static_cast<float>(_lastFrameDuration) / 1000000.0f;
	}

	// Accessor methods

	/**
	 * Returns the width of the output buffer in pixels
	 */
	int getDisplayWidth() const {
		return _width;
	}

	/**
	 * Returns the height of the output buffer in pixels
	 */
	int getDisplayHeight() const {
		return _height;
	}

	/**
	 * Returns the bounding box of the output buffer: (0, 0, Width, Height)
	 */
	Common::Rect &getDisplayRect() {
		return _screenRect;
	}

	/**
	 * Returns the bit depth of the output buffer
	 */
	int getBitDepth() {
		return _bitDepth;
	}

	/**
	 * Determines whether the frame buffer change is to be synchronised with Vsync. This is turned on by default.
	 * Notes: In windowed mode, this setting has no effect.
	 * @param Vsync     Indicates whether the frame buffer changes are to be synchronised with Vsync.
	 */
	void setVsync(bool vsync);

	/**
	 * Returns true if V-Sync is on.
	 * Notes: In windowed mode, this setting has no effect.
	 */
	bool getVsync() const;

	/**
	 * Fills a rectangular area of the frame buffer with a color.
	 * Notes: It is possible to create transparent rectangles by passing a color with an Alpha value of 255.
	 * @param FillRectPtr   Pointer to a Common::Rect, which specifies the section of the frame buffer to be filled.
	 * If the rectangle falls partly off-screen, then it is automatically trimmed.
	 * If a NULL value is passed, then the entire image is to be filled.
	 * @param Color         The 32-bit color with which the area is to be filled. The default is BS_RGB(0, 0, 0) (black)
	 * @note FIf the rectangle is not completely inside the screen, it is automatically clipped.
	 */
	bool fill(const Common::Rect *fillRectPtr = 0, uint color = BS_RGB(0, 0, 0));

	Graphics::Surface _backSurface;
	Graphics::Surface *getSurface() { return &_backSurface; }

	Common::SeekableReadStream *_thumbnail;
	Common::SeekableReadStream *getThumbnail() { return _thumbnail; }

	// Access methods

	// Resource-Managing Methods
	// --------------------------
	Resource *loadResource(const Common::String &fileName) override;
	bool canLoadResource(const Common::String &fileName) override;

	// Persistence Methods
	// -------------------
	bool persist(OutputPersistenceBlock &writer) override;
	bool unpersist(InputPersistenceBlock &reader) override;

	static void ARGBColorToLuaColor(lua_State *L, uint color);
	static uint luaColorToARGBColor(lua_State *L, int stackIndex);

protected:

	// Display Variables
	// -----------------
	int _width;
	int _height;
	Common::Rect _screenRect;
	int _bitDepth;

	/**
	 * Calculates the time since the last frame beginning has passed.
	 */
	void updateLastFrameDuration();

private:
	bool registerScriptBindings();
	void unregisterScriptBindings();

	// LastFrameDuration Variables
	// ---------------------------
	uint _lastTimeStamp;
	uint _lastFrameDuration;
	bool _timerActive;
	Common::Array<uint> _frameTimeSamples;
	uint _frameTimeSampleSlot;

private:
	RenderObjectPtr<Panel> _mainPanelPtr;

	Common::ScopedPtr<RenderObjectManager> _renderObjectManagerPtr;

	struct DebugLine {
		DebugLine(const Vertex &start, const Vertex &end, uint color) :
			_start(start),
			_end(end),
			_color(color) {}
		DebugLine() {}

		Vertex _start;
		Vertex _end;
		uint _color;
	};
};

} // End of namespace Sword25

#endif
