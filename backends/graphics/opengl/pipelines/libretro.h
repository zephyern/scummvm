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

#ifndef BACKENDS_GRAPHICS_OPENGL_PIPELINES_LIBRETRO_H
#define BACKENDS_GRAPHICS_OPENGL_PIPELINES_LIBRETRO_H

#include "graphics/opengl/system_headers.h"

#if !USE_FORCED_GLES
#include "backends/graphics/opengl/pipelines/shader.h"

#include "common/array.h"
#include "common/fs.h"

namespace Graphics {
struct Surface;
}

namespace OpenGL {

namespace LibRetro {
struct ShaderPreset;
struct ShaderPass;
} // End of namespace LibRetro

class TextureTarget;

/**
 * Pipeline implementation using Libretro shader presets.
 */
class LibRetroPipeline : public ShaderPipeline {
public:
	LibRetroPipeline();
	LibRetroPipeline(const Common::FSNode &shaderPreset);
	virtual ~LibRetroPipeline();

	virtual void drawTexture(const GLTexture &texture, const GLfloat *coordinates, const GLfloat *texcoords);

	virtual void setProjectionMatrix(const GLfloat *projectionMatrix);

	void setOutputSize(uint outputWidth, uint outputHeight);

	bool open(const Common::FSNode &shaderPreset);
	void close();

	bool isInitialized() const { return _shaderPreset != nullptr; }

	static bool isSupportedByContext() {
		return OpenGLContext.shadersSupported
			&& OpenGLContext.multitextureSupported
			&& OpenGLContext.framebufferObjectSupported;
	}
private:
	virtual void activateInternal();
	virtual void deactivateInternal();

	bool loadTextures();
	bool loadPasses();

	void setPipelineState();
	bool setupFBOs();
	void setupPassUniforms(const uint id);
	void setShaderTexUniforms(const Common::String &prefix, Shader *shader, const GLTexture &texture);

	const LibRetro::ShaderPreset *_shaderPreset;

	bool _applyProjectionChanges;

	uint _inputWidth;
	uint _inputHeight;

	bool _outputSizeChanged;
	uint _outputWidth;
	uint _outputHeight;

	uint _frameCount;

	struct Texture {
		Texture() : textureData(nullptr), glTexture(nullptr) {}
		Texture(Graphics::Surface *tD, GLTexture *glTex) : textureData(tD), glTexture(glTex) {}

		Common::String id;
		Graphics::Surface *textureData;
		GLTexture *glTexture;
	};
	Texture loadTexture(const Common::FSNode &fileNode);

	typedef Common::Array<Texture> TextureArray;
	TextureArray _textures;

	struct Pass {
		Pass()
			: shaderPass(nullptr), shader(nullptr), target(nullptr),
			  texCoords(), texSamplers(), inputTexture(nullptr), vertexCoord(), hasFrameCount(false) {}
		Pass(const LibRetro::ShaderPass *sP, Shader *s, TextureTarget *t)
			: shaderPass(sP), shader(s), target(t), texCoords(),
			  texSamplers(), inputTexture(nullptr), vertexCoord(), hasFrameCount(false) {}

		const LibRetro::ShaderPass *shaderPass;
		Shader *shader;
		TextureTarget *target;

		/**
		 * Description of texture coordinates bound to attribute.
		 */
		struct TexCoordAttribute {
			/**
			 * Attribute name to bind data to.
			 */
			Common::String name;

			enum Type {
				/**
				 * 'index' denotes the 'index'th shader texture's coordinates.
				 */
				kTypeTexture,

				/**
				 * 'index' denotes the texture coordinates given to pass 'index'.
				 */
				kTypePass,

				/**
				 * 'index' denotes the texture coordinates of the 'index'th previous frame.
				 */
				kTypePrev
			};

			/**
			 * The type of the attribute.
			 */
			Type type;

			/**
			 * Index for the texture coordinates to use.
			 */
			uint index;

			TexCoordAttribute() : name(), type(), index() {}
			TexCoordAttribute(const Common::String &n, Type t, uint i) : name(n), type(t), index(i) {}
		};

		typedef Common::Array<TexCoordAttribute> TexCoordAttributeArray;
		TexCoordAttributeArray texCoords;

		/**
		 * Build the 'texCoords' array.
		 *
		 * @param id Identifier of the current pass.
		 */
		void buildTexCoords(const uint id, const Common::StringArray &aliases);

		void addTexCoord(const Common::String &prefix, const TexCoordAttribute::Type type, const uint index);

		/**
		 * Description of a texture sampler.
		 */
		struct TextureSampler {
			/**
			 * Texture unit to use.
			 */
			uint unit;

			enum Type {
				/**
				 * 'index' denotes the 'index'th shader texture.
				 */
				kTypeTexture,

				/**
				 * 'index' denotes the input to pass 'index'.
				 */
				kTypePass,

				/**
				 * 'index' denotes the input of the 'index'th previous frame.
				 */
				kTypePrev
			};

			/**
			 * Source type of the texture to bind.
			 */
			Type type;

			/**
			 * Index of the texture.
			 */
			uint index;

			TextureSampler() : unit(), type(), index() {}
			TextureSampler(uint u, Type t, uint i) : unit(u), type(t), index(i) {}
		};

		typedef Common::Array<TextureSampler> TextureSamplerArray;
		TextureSamplerArray texSamplers;

		/**
		 * Build the 'texSamplers' array.
		 *
		 * @param id       Identifier of the current pass.
		 * @param textures Array of shader textures available.
		 */
		void buildTexSamplers(const uint id, const TextureArray &textures, const Common::StringArray &aliases);

		void addTexSampler(const Common::String &name, uint *unit, const TextureSampler::Type type, const uint index, const bool prefixIsId = false);

		/**
		 * Input texture of the pass.
		 */
		const GLTexture *inputTexture;

		/**
		 * Vertex coordinates used for drawing.
		 */
		GLfloat vertexCoord[2*4];

		/**
		 * Whether the shader has a FrameCount uniform or not
		 * Allows to speed up if it is not here
		 */
		bool hasFrameCount;
	};

	typedef Common::Array<Pass> PassArray;
	PassArray _passes;

	void renderPass(const Pass &pass);
	void renderPassSetupCoordinates(const Pass &pass);
	void renderPassSetupTextures(const Pass &pass);
};

} // End of namespace OpenGL
#endif // !USE_FORCED_GLES

#endif
