#pragma once

namespace cinder {
    namespace openni {
        namespace primesense {
#include "OpenNI.h"
        }
        namespace _openni = primesense::openni;


        class ImageSourceColor : public ci::ImageSource
        {
        public:
            ImageSourceColor( _openni::RGB888Pixel *buffer, int width, int height )
            : ci::ImageSource(), mData( buffer ), _width(width), _height(height)
            {
                setSize( _width, _height );
                setColorModel( ImageIo::CM_RGB );
                setChannelOrder( ImageIo::RGB );
                setDataType( ImageIo::UINT8 );
            }

            ~ImageSourceColor()
            {
            }

            virtual void load( ci::ImageTargetRef target )
            {
                ImageSource::RowFunc func = setupRowFunc( target );

                for( uint32_t row	 = 0; row < _height; ++row )
                    ((*this).*func)( target, row, mData + row * _width );
            }

        protected:
            uint32_t					_width, _height;
            _openni::RGB888Pixel		*mData;
        };


        class ImageSourceDepth : public ci::ImageSource
        {
        public:
            ImageSourceDepth( _openni::DepthPixel *buffer, int width, int height )
            : ci::ImageSource(), mData( buffer ), _width(width), _height(height)
            {
                setSize( _width, _height );
                setColorModel( ci::ImageIo::CM_GRAY );
                setChannelOrder( ci::ImageIo::Y );
                setDataType( ci::ImageIo::UINT16 );
            }

            ~ImageSourceDepth()
            {
            }

            virtual void load( ci::ImageTargetRef target )
            {
                ci::ImageSource::RowFunc func = setupRowFunc( target );

                for( uint32_t row = 0; row < _height; ++row )
                    ((*this).*func)( target, row, mData + row * _width );
            }
            
        protected:
            uint32_t					_width, _height;
            _openni::DepthPixel			*mData;
        };

    }
}


#include "cinder/Cinder.h"
#include "CinderOpenNI/Camera.h"
