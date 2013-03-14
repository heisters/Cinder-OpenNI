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
            ImageSourceRef getColorImage();
            gl::Texture & getDepthTex();
            gl::Texture & getColorTex();
            Vec2i getDepthSize(){ return getFrame( depthIndex ).size; }
            Vec2i getColorSize(){ return getFrame( colorIndex ).size; }

            enum SENSORS {
                SENSOR_DEPTH = 0x1,
                SENSOR_COLOR = 0x2
            };
        private:

            _openni::Device device;
            _openni::VideoStream depthStream, colorStream;
            int depthIndex, colorIndex;

            struct Frame {
                Frame( _openni::VideoStream &stream, Vec2i size );
                Vec2i size;
                _openni::VideoStream &stream;
                _openni::VideoFrameRef frameRef;
                ImageSourceRef imageRef;
                gl::Texture tex;
                bool isImageFresh, isTexFresh;


                template < typename pixel_t, typename image_t >
                void updateImage();
                template < typename pixel_t, typename image_t >
                void updateTex();
            };

            std::vector< Frame > all;
            _openni::VideoStream  **allStreams;

            int setupStream( _openni::VideoStream &stream, _openni::SensorType sensorType );
            void updateStream( int streamIndex );
            Frame & getFrame( int index );
        };
    }
}