#pragma once
#include "core/types/resource.h"
#include <vector>
namespace resources {
	class Image : public Resource {
		GUS_DECLARE_CLASS(Image, Resource)
		
		enum class ImageFormat {
			FORMAT_ERR,

			FORMAT_8L,
			FORMAT_8LA,
			FORMAT_8RGB,
			FORMAT_8RGBA,
			FORMAT_16L,
			FORMAT_16LA,
			FORMAT_16RGB,
			FORMAT_16RGBA
		};

		private:

		vector<uint8_t> _pixelData {};
		int32_t _width, _height, _channels = 0;
		ImageFormat _format = ImageFormat::FORMAT_ERR;

		public:

		ImageFormat GetFormat() const {return _format; };
		int32_t GetWidth() const {return _width; };
		int32_t GetHeight() const {return _height; };
		int32_t GetChannels() const {return _channels; };
		bool Is16Bit() const {
			return (_format == ImageFormat::FORMAT_16L || _format == ImageFormat::FORMAT_16LA || _format == ImageFormat::FORMAT_16RGB || _format == ImageFormat::FORMAT_16RGBA);
		}


		static Image* CreateFromFile(string filePath);


		~Image() override;
	};

}