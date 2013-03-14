#include "CinderOpenNI.h"
#include "cinder/app/AppBasic.h"



namespace cinder { namespace openni {
    void handleStatus(_openni::Status &status, std::string message)
    {
        if ( status == _openni::STATUS_OK ) return;

        app::console() << message << ": " << _openni::OpenNI::getExtendedError() << std::endl;
        Camera::shutdown();
        throw 1;
    }


    bool Camera::initialized = false;

    void Camera::initialize()
    {
        if ( initialized ) return;
        _openni::Status status = _openni::STATUS_OK;
        status = _openni::OpenNI::initialize();
        handleStatus(status, "Error initializing OpenNI");
        initialized = true;
    }

    void Camera::shutdown()
    {
        if ( !initialized ) return;

        _openni::OpenNI::shutdown();
        initialized = false;
    }

    Camera::Camera() :
    allStreams(NULL)
    {}

    Camera::~Camera()
    {
        close();
        shutdown();

        if ( allStreams != NULL ) {
            delete []allStreams;
        }
    }

    int Camera::setupStream(_openni::VideoStream &stream, _openni::SensorType sensorType )
    {
        _openni::Status status = _openni::STATUS_OK;

        std::string description = sensorType == _openni::SENSOR_COLOR ? "color stream" : "depth stream";

        status = stream.create(device, sensorType);
        handleStatus( status, "Could not find " + description );

        status = stream.start();
        if ( status != _openni::STATUS_OK ) stream.destroy();
        handleStatus( status, "Could not start " + description );


        if ( !stream.isValid() ) {
            app::console() << description << " is not valid.";
            throw 1;
        }


        _openni::VideoMode mode = depthStream.getVideoMode();
        Vec2i size = Vec2i( mode.getResolutionX(), mode.getResolutionY() );

        int index = all.size();
        all.push_back( Frame( stream, size ) );
        allStreams[index] = &stream;

        return index;
    }

    void Camera::setup(int enableSensors)
    {
        _openni::Status status = _openni::STATUS_OK;


        initialize();

        status = device.open(_openni::ANY_DEVICE);
        handleStatus( status, "Could not open device" );

        allStreams = new _openni::VideoStream*[2];
        allStreams[0] = NULL;
        allStreams[1] = NULL;


        if ( (enableSensors & SENSOR_DEPTH) == SENSOR_DEPTH ) {
            depthIndex = setupStream( depthStream, _openni::SENSOR_DEPTH );
        }

        if ( (enableSensors & SENSOR_COLOR) == SENSOR_COLOR ) {
            colorIndex = setupStream( colorStream, _openni::SENSOR_COLOR );
        }

        if ( !depthStream.isValid() && !colorStream.isValid() ) {
            app::console() << "No valid OpenNI streams." << std::endl;
            throw 1;
        }
    }

    void Camera::update()
    {
        int changedStreamIndex;

        _openni::Status status = _openni::OpenNI::waitForAnyStream(allStreams, all.size(), &changedStreamIndex);
        if ( status != _openni::STATUS_OK ) {
            app::console() << "Waiting for new OpenNI data failed." << std::endl;
            return;
        }

        updateStream( changedStreamIndex );
    }

    void Camera::updateStream( int streamIndex )
    {
        Frame &frame = getFrame( streamIndex );
        frame.stream.readFrame( &frame.frameRef );
        frame.isImageFresh = false;
        frame.isTexFresh = false;
    }

    void Camera::close()
    {
        for ( auto &f : all ) {
            f.stream.stop();
            f.stream.destroy();
            device.close();
        }
    }

    ImageSourceRef Camera::getDepthImage()
    {
        Frame &frame = getFrame( depthIndex );
        frame.updateImage< _openni::DepthPixel *, ImageSourceDepth >();
        return frame.imageRef;
    }

    ImageSourceRef Camera::getColorImage()
    {
        Frame &frame = getFrame( colorIndex );
        frame.updateImage< _openni::RGB888Pixel *, ImageSourceColor >();
        return frame.imageRef;
    }

    gl::Texture & Camera::getDepthTex()
    {
        Frame &frame = getFrame( depthIndex );
        frame.updateTex< _openni::DepthPixel *, ImageSourceDepth >();
        return frame.tex;
    }

    gl::Texture & Camera::getColorTex()
    {
        Frame &frame = getFrame( colorIndex );
        frame.updateTex< _openni::RGB888Pixel *, ImageSourceColor >();
        return frame.tex;
    }

    Camera::Frame & Camera::getFrame( int index )
    {
        return all.at( index );
    }

    Camera::Frame::Frame( _openni::VideoStream &stream, Vec2i size ) :
    stream(stream), size(size), isImageFresh(false), isTexFresh(false),
    imageRef(Surface8u(size.x, size.y, false))
    {

    }

    template < typename pixel_t, typename image_t >
    void Camera::Frame::updateImage()
    {
        if ( isImageFresh || !frameRef.isValid() ) return;

        pixel_t data = (pixel_t)frameRef.getData();
        imageRef = ImageSourceRef( new image_t( data, size.x, size.y ) );
        isImageFresh = true;
    }

    template < typename pixel_t, typename image_t >
    void Camera::Frame::updateTex()
    {
        if ( isTexFresh ) return;

        updateImage< pixel_t, image_t >();
        tex = gl::Texture( imageRef );
        isTexFresh = true;
    }
} }