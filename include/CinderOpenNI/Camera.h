#pragma once

#include "cinder/gl/Texture.h"

namespace cinder {
    namespace openni {
        class Camera {
        public:
            Camera();
            ~Camera();

            static bool initialized;
            static void initialize();
            static void shutdown();

            void setup(int enableSensors=SENSOR_DEPTH|SENSOR_COLOR);
            void update();
            void close();

            ImageSourceRef getDepthImage();
            ImageSourceRef getRawDepthImage();
            ImageSourceRef getColorImage();
            gl::Texture & getDepthTex();
            gl::Texture & getRawDepthTex();
            gl::Texture & getColorTex();
            Vec2i getDepthSize(){ return getFrameData( depthIndex ).size; }
            Vec2i getColorSize(){ return getFrameData( colorIndex ).size; }

            enum SENSORS {
                SENSOR_DEPTH = 0x1,
                SENSOR_COLOR = 0x2
            };

			class CameraException : public std::exception {
			};
        private:

            _openni::Device device;
            _openni::VideoStream depthStream, colorStream;
            int depthIndex, colorIndex;

            class FrameDataAbstract {
            public:
                FrameDataAbstract( Vec2i size );

                Vec2i size;
                gl::Texture tex;
                bool isImageFresh, isTexFresh;
            };

            class FrameData : public FrameDataAbstract {
            public:
                FrameData( _openni::VideoStream &stream, Vec2i size );

                _openni::VideoStream &stream;
                _openni::VideoFrameRef frameRef;
                ImageSourceRef imageRef;

                template < typename pixel_t, typename image_t >
                void updateImage();
                template < typename pixel_t, typename image_t >
                void updateTex();
            };

            class DerivedFrameData : public FrameDataAbstract {
            public:
                DerivedFrameData();
                DerivedFrameData( FrameData *original );

                Surface8u image;

                template < typename pixel_t, typename image_t, typename original_pixel_t, typename original_image_t >
                void updateImage();
                template < typename pixel_t, typename image_t, typename original_pixel_t, typename original_image_t >
                void updateTex();
            private:
                void convertData( SurfaceT< _openni::DepthPixel > &originalImage, Surface8u &convertedImage );
                FrameData *original;
            };

            std::vector< FrameData > all;
            _openni::VideoStream  **allStreams;
            DerivedFrameData scaledDepthFrameData;

            int setupStream( _openni::VideoStream &stream, _openni::SensorType sensorType );
            void updateStream( int streamIndex );
            FrameData & getFrameData( int index );
        };
    }
}