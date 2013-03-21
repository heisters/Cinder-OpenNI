#pragma once

#include "cinder/ImageIo.h"
#include "OpenNI.h"

namespace cinder {
    namespace openni {
        namespace _openni = ::openni;

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


        class ImageSourceRawDepth : public ci::ImageSource
        {
        public:
            ImageSourceRawDepth( _openni::DepthPixel *buffer, int width, int height )
            : ci::ImageSource(), mData( buffer ), _width(width), _height(height)
            {
                setSize( _width, _height );
                setColorModel( ci::ImageIo::CM_GRAY );
                setChannelOrder( ci::ImageIo::Y );
                setDataType( ci::ImageIo::UINT16 );
            }

            ~ImageSourceRawDepth()
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

        class ImageSourceDepth : public ci::ImageSource
        {
        public:
            ImageSourceDepth( uint8_t *buffer, int width, int height )
            : ci::ImageSource(), mData( buffer ), _width(width), _height(height)
            {
                setSize( _width, _height );
                setColorModel( ci::ImageIo::CM_GRAY );
                setChannelOrder( ci::ImageIo::Y );
                setDataType( ci::ImageIo::UINT8 );
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
            uint8_t                     *mData;
        };
    }
}
#include "CinderOpenNI/Camera.h"
