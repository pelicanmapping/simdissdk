/* -*- mode: c++ -*- */
/****************************************************************************
 *****                                                                  *****
 *****                   Classification: UNCLASSIFIED                   *****
 *****                    Classified By:                                *****
 *****                    Declassify On:                                *****
 *****                                                                  *****
 ****************************************************************************
 *
 *
 * Developed by: Naval Research Laboratory, Tactical Electronic Warfare Div.
 *               EW Modeling & Simulation, Code 5773
 *               4555 Overlook Ave.
 *               Washington, D.C. 20375-5339
 *
 * License for source code at https://simdis.nrl.navy.mil/License.aspx
 *
 * The U.S. Government retains all rights to use, duplicate, distribute,
 * disclose, or release this software.
 *
 */
#ifndef SIMDIS_DB_LAYERS_H
#define SIMDIS_DB_LAYERS_H 1

#include "osgEarth/Version"

#include <string>
#include "osgEarth/ImageLayer"
#include "osgEarth/ElevationLayer"
#include "osgEarth/URI"
#include "simCore/Common/Common.h"
#include "simCore/Common/Export.h"
#include "simCore/Time/TimeClass.h"
#include "simVis/osgEarthVersion.h"
#include "sqlite3.h"

#include "simVis/DBOptions.h"


namespace simVis
{
  using namespace osgEarth;

  //........................................................................

  class SDKVIS_EXPORT DBImageLayer : public ImageLayer
  {
  public: // serialization
    class SDKVIS_EXPORT Options : public ImageLayer::Options {
    public:
      META_LayerOptions(simVis, Options, ImageLayer::Options);
      OE_OPTION(DBOptions, driver);
      virtual Config getConfig() const;
    private:
      void fromConfig(const Config&);
    };

  public:
    META_Layer(simVis, DBImageLayer, Options, ImageLayer, DBImage);

  public:
    //! Base URL of TileCache endpoint
    void setURL(const URI& value);
    const URI& getURL() const;

    //! Maximum level to use in DB file
    void setDeepestLevel(const unsigned& value);
    const unsigned& getDeepestLevel() const;

  public: // Layer

    //! Establishes a connection to the database
    virtual Status openImplementation();

    //! Creates a raster image for the given tile key
    virtual GeoImage createImageImplementation(const TileKey& key, ProgressCallback* progress) const;

  protected: // Layer

    //! Called by constructors
    virtual void init();

    //! Create and return the underlying TileSource
    virtual osgEarth::TileSource* createTileSource();

    //! Destructor
    virtual ~DBImageLayer() { }
  };

  /**
    * Elevation layer connected to a DB file
    */
  class SDKVIS_EXPORT DBElevationLayer : public ElevationLayer
  {
  public: // serialization
    class SDKVIS_EXPORT Options : public ElevationLayer::Options {
    public:
      META_LayerOptions(simVis, Options, ElevationLayer::Options);
      OE_OPTION(DBOptions, driver);
      virtual Config getConfig() const;
    private:
      void fromConfig(const Config&);
    };

  public:
    META_Layer(simVis, DBElevationLayer, Options, ElevationLayer, DBElevation);

    //! Base URL of TileCache endpoint
    void setURL(const URI& value);
    const URI& getURL() const;

    //! Maximum level to use in DB file
    void setDeepestLevel(const unsigned& value);
    const unsigned& getDeepestLevel() const;

  public: // Layer

    //! Establishes a connection to the TMS repository
    virtual Status openImplementation();

    //! Creates a heightfield for the given tile key
    virtual GeoHeightField createHeightFieldImplementation(const TileKey& key, ProgressCallback* progress) const;

  protected: // Layer

      //! Called by constructors
    virtual void init();

    //! Create and return the underlying TileSource
    virtual osgEarth::TileSource* createTileSource();

    //! Destructor
    virtual ~DBElevationLayer() { }
  };

}

#endif // SIMDIS_DB_LAYERS_H
