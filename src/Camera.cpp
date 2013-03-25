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

    /**************************************************************************
     * getters
     */
    ImageSourceRef Camera::getDepthImage()
    {
        scaledDepthFrameData.updateOriginal( &getFrameData(depthIndex) );
        scaledDepthFrameData.updateImage< uint8_t, ImageSourceDepth, _openni::DepthPixel >();
        return scaledDepthFrameData.imageRef;
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
        scaledDepthFrameData.updateOriginal( &getFrameData(depthIndex) );
        scaledDepthFrameData.updateTex< uint8_t, ImageSourceDepth, _openni::DepthPixel >();
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

    /**************************************************************************
     * FrameDataAbstract
     */
    Camera::FrameDataAbstract::FrameDataAbstract( Vec2i size ) :
    isImageFresh( false ), isTexFresh( false ),
	size( size )
    {
    }

    void Camera::FrameDataAbstract::initTexture( Vec2i _size )
    {
        imageRef = ImageSourceRef( Surface8u(_size.x, _size.y, false) );
        tex = gl::Texture( imageRef );
    }

    /**************************************************************************
     * FrameData
     */
    Camera::FrameData::FrameData( _openni::VideoStream &stream, Vec2i size ) :
    stream(stream),
    FrameDataAbstract( size )
    {
        initTexture(size);
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

	/**************************************************************************
     * DerivedFrameData
     */
    Camera::DerivedFrameData::DerivedFrameData() :
    original( NULL ),
    FrameDataAbstract( Vec2i::zero() ),
    convertedData( NULL )
    {
    }

    Camera::DerivedFrameData::~DerivedFrameData()
    {
        if ( convertedData != NULL ) delete []convertedData;
    }

    void Camera::DerivedFrameData::updateOriginal( FrameData *_original )
    {
        if ( original == NULL ) {
            initTexture( _original->size );
        }

        size = _original->size;
        original = _original;
    }

    template < typename pixel_t, typename image_t, typename original_pixel_t >
    void Camera::DerivedFrameData::updateImage()
    {
        if ( isImageFresh || original == NULL || !original->frameRef.isValid() ) return;

        original_pixel_t *originalData = (original_pixel_t *)original->frameRef.getData();
        convertData( originalData, &convertedData );
        imageRef = ImageSourceRef( new image_t( convertedData, size.x, size.y ) );

        isImageFresh = true;
    }

    template < typename pixel_t, typename image_t, typename original_pixel_t >
    void Camera::DerivedFrameData::updateTex()
    {
        if ( isTexFresh ) return;

        updateImage< pixel_t, image_t, original_pixel_t >();
        tex = gl::Texture( imageRef );
        isTexFresh = true;
    }

    void Camera::DerivedFrameData::convertData( const _openni::DepthPixel *originalData, uint8_t **_convertedData )
    {
        int _size = size.x * size.y;
        float scale = (255.0 / (float)original->stream.getMaxPixelValue());
        if ( *_convertedData == NULL ) *_convertedData = new uint8_t[_size];

        for ( int i = 0; i < _size; ++i ) {
            (*_convertedData)[i] = (uint8_t)(originalData[i] * scale);
        }
    }

} }