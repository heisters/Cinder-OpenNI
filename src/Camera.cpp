#include "CinderOpenNI.h"
#include "CinderOpenNI/Camera.h"
#include "cinder/app/AppBasic.h"


namespace cinder { namespace openni {
    void handleStatus(_openni::Status &status, std::string message)
    {
        if ( status == _openni::STATUS_OK ) return;

        app::console() << message << ": " << _openni::OpenNI::getExtendedError() << std::endl;
        Camera::shutdown();
		throw Camera::CameraException();
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
            throw Camera::CameraException();
        }


        _openni::VideoMode mode = stream.getVideoMode();
        Vec2i size = Vec2i( mode.getResolutionX(), mode.getResolutionY() );

        int index = all.size();
        all.push_back( FrameData( stream, size ) );
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
            scaledDepthFrameData = DerivedFrameData( &getFrameData(depthIndex) );
        }

        if ( (enableSensors & SENSOR_COLOR) == SENSOR_COLOR ) {
            colorIndex = setupStream( colorStream, _openni::SENSOR_COLOR );
        }

        if ( !depthStream.isValid() && !colorStream.isValid() ) {
            app::console() << "No valid OpenNI streams." << std::endl;
            throw Camera::CameraException();
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
        FrameData &frame = getFrameData( streamIndex );
        frame.stream.readFrame( &frame.frameRef );
        frame.isImageFresh = false;
        frame.isTexFresh = false;

        // FIXME: not so nice :(
        if ( streamIndex == depthIndex ) {
            scaledDepthFrameData.isImageFresh = false;
            scaledDepthFrameData.isTexFresh = false;
        }
    }

    void Camera::close()
    {
        for ( auto &f : all ) {
            f.stream.stop();
            f.stream.destroy();
        }
        device.close();
    }

    ImageSourceRef Camera::getDepthImage()
    {
        scaledDepthFrameData.updateImage< uint8_t, ImageSourceDepth, _openni::DepthPixel, ImageSourceRawDepth >();
        return scaledDepthFrameData.image;
    }

    ImageSourceRef Camera::getRawDepthImage()
    {
        FrameData &frame = getFrameData( depthIndex );
        frame.updateImage< _openni::DepthPixel, ImageSourceRawDepth >();
        return frame.imageRef;
    }

    ImageSourceRef Camera::getColorImage()
    {
        FrameData &frame = getFrameData( colorIndex );
        frame.updateImage< _openni::RGB888Pixel, ImageSourceColor >();
        return frame.imageRef;
    }

    gl::Texture & Camera::getDepthTex()
    {
        scaledDepthFrameData.updateTex< uint8_t, ImageSourceDepth, _openni::DepthPixel, ImageSourceRawDepth >();
        return scaledDepthFrameData.tex;
    }

    gl::Texture & Camera::getRawDepthTex()
    {
        FrameData &frame = getFrameData( depthIndex );
        frame.updateTex< _openni::DepthPixel, ImageSourceRawDepth >();
        return frame.tex;
    }

    gl::Texture & Camera::getColorTex()
    {
        FrameData &frame = getFrameData( colorIndex );
        frame.updateTex< _openni::RGB888Pixel, ImageSourceColor >();
        return frame.tex;
    }

    Camera::FrameData & Camera::getFrameData( int index )
    {
        return all.at( index );
    }

    Camera::FrameDataAbstract::FrameDataAbstract( Vec2i size ) :
    isImageFresh( false ), isTexFresh( false ),
	size( size )
    {
    }

    Camera::FrameData::FrameData( _openni::VideoStream &stream, Vec2i size ) :
    stream(stream),
    FrameDataAbstract( size ),
    imageRef(Surface8u(size.x, size.y, false))
    {
    }

    template < typename pixel_t, typename image_t >
    void Camera::FrameData::updateImage()
    {
        if ( isImageFresh || !frameRef.isValid() ) return;

        pixel_t *data = (pixel_t *)frameRef.getData();
        imageRef = ImageSourceRef( new image_t( data, size.x, size.y ) );
        isImageFresh = true;
    }

    template < typename pixel_t, typename image_t >
    void Camera::FrameData::updateTex()
    {
        if ( isTexFresh ) return;

        updateImage< pixel_t, image_t >();
        tex = gl::Texture( imageRef );
        isTexFresh = true;
    }

    Camera::DerivedFrameData::DerivedFrameData() :
    original(NULL),
    FrameDataAbstract( Vec2i::zero() )
    {
    }

    Camera::DerivedFrameData::DerivedFrameData( FrameData *original ) :
    original(original),
    FrameDataAbstract( original->size ),
    image(size.x, size.y, false)
    {
    }

    template < typename pixel_t, typename image_t, typename original_pixel_t, typename original_image_t >
    void Camera::DerivedFrameData::updateImage()
    {
        if ( isImageFresh ) return;

        original->updateImage< original_pixel_t, original_image_t >();

        SurfaceT< original_pixel_t > originalImage(original->imageRef, SurfaceConstraintsDefault(), boost::tribool::false_value);
        convertData( originalImage, image );
        isImageFresh = true;
    }

    template < typename pixel_t, typename image_t, typename original_pixel_t, typename original_image_t >
    void Camera::DerivedFrameData::updateTex()
    {
        if ( isTexFresh ) return;

        updateImage< pixel_t, image_t, original_pixel_t, original_image_t >();
        tex = gl::Texture( image );
        isTexFresh = true;
    }

    void Camera::DerivedFrameData::convertData( SurfaceT< _openni::DepthPixel > &originalImage, Surface8u &convertedImage )
    {
        int size = original->size.x * original->size.y;
        float scale = (255.0 / (float)original->stream.getMaxPixelValue());

        Surface8u::Iter it = convertedImage.getIter();
        while ( it.line() ) {
            while ( it.pixel() ) {
                ColorT< _openni::DepthPixel >color = originalImage.getPixel( it.getPos() );
                Color8u convertedPixel( color.r * scale, color.g * scale, color.b * scale );
                convertedImage.setPixel( it.getPos(), convertedPixel );
            }
        }
    }
} }