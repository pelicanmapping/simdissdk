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
 * License for source code is in accompanying LICENSE.txt file. If you did
 * not receive a LICENSE.txt with this code, email simdis@us.navy.mil.
 *
 * The U.S. Government retains all rights to use, duplicate, distribute,
 * disclose, or release this software.
 *
 */

/**
* PERISCOPE EXAMPLE - SIMDIS SDK
*/
#include "simCore/Common/Version.h"
#include "simCore/Common/HighPerformanceGraphics.h"
#include "simNotify/Notify.h"
#include "simCore/Calc/Angle.h"
#include "simCore/Calc/Coordinate.h"
#include "simCore/Calc/CoordinateConverter.h"
#include "simData/MemoryDataStore.h"

#include "simVis/osgEarthVersion.h"
#include "simVis/Platform.h"
#include "simVis/PlatformModel.h"
#include "simVis/Registry.h"
#include "simVis/Scenario.h"
#include "simVis/ScenarioDataStoreAdapter.h"
#include "simVis/SceneManager.h"
#include "simVis/Viewer.h"
#include "simUtil/ExampleResources.h"
#include "simUtil/HudManager.h"

#include "osgDB/ReadFile"
#include "osgEarth/SimpleOceanLayer"
#include "osgEarth/Version"

#ifdef HAVE_OSGEARTH_TRITON
#include "osgEarthTriton/TritonLayer"
#endif

#ifdef HAVE_OSGEARTH_SILVERLINING
#include "osgEarthSilverLining/SilverLiningNode"
#endif

#define SHIP_LAT        19.698193
#define SHIP_LON      -156.20224
#define PLATFORM_SHIP "Ship"

#define SUB_LAT          19.69
#define SUB_LON        -156.203
#define SUB_ALT         3.5

using namespace osgEarth::Util;

// put a ship out there.
static simCore::Coordinate s_shipPosOri(simCore::COORD_SYS_LLA,
  simCore::Vec3(simCore::DEG2RAD*(SHIP_LAT), simCore::DEG2RAD*(SHIP_LON), 0),
  simCore::Vec3(0.0, 0.0, 0.0));

static simData::ObjectId                  s_shipId;


double g_eyeLat, g_eyeLon, g_eyeAlt, g_eyeAz, g_eyeEl;

namespace
{
  osg::Matrix lookAt(double lat0, double lon0, double alt0, double az, double el)
  {
    osgEarth::SpatialReference* WGS84 = osgEarth::SpatialReference::get("wgs84");

    osgEarth::GeoPoint from(WGS84, lon0, lat0, alt0, osgEarth::ALTMODE_ABSOLUTE);
    osg::Matrix local2world;
    from.createLocalToWorld(local2world);

    osg::Quat azQ(simCore::DEG2RAD*az, -osg::Vec3(0, 0, 1));
    osg::Quat elQ(simCore::DEG2RAD*el,  osg::Vec3(1, 0, 0));
    osg::Vec3d lookVec = elQ * azQ * osg::Vec3d(0, 1, 0);

    osg::Vec3d fromECEF = osg::Vec3d(0, 0, 0) * local2world;
    osg::Vec3d toECEF   = lookVec * local2world;
    return osg::Matrix::lookAt(fromECEF, toECEF, fromECEF);
  }

  struct ViewControls : public osgGA::GUIEventHandler
  {
    bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
    {
      if (ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN)
      {
        switch (ea.getKey())
        {
        case osgGA::GUIEventAdapter::KEY_Left:
          slew(-1, 0, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_Right:
          slew(+1, 0, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_Up:
          slew(0, +1, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_Down:
          slew(0, -1, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_A:
          height(+1, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_Z:
          height(-1, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_Page_Up:
          height(+10, aa);
          break;
        case osgGA::GUIEventAdapter::KEY_Page_Down:
          height(-10, aa);
          break;
        }
      }

      if (ea.getEventType() == osgGA::GUIEventAdapter::FRAME)
      {
        osg::Matrix lookat = lookAt(g_eyeLat, g_eyeLon, g_eyeAlt, g_eyeAz, g_eyeEl);
        aa.asView()->getCamera()->setViewMatrix(lookat);
      }

      return false;
    }

    void slew(int deltaAz, int deltaEl, osgGA::GUIActionAdapter& aa)
    {
      osg::View* view = aa.asView();
      if (view)
      {
        g_eyeAz = g_eyeAz + 2.0*(double)deltaAz;
        g_eyeEl = g_eyeEl + 1.0*(double)deltaEl;
        aa.requestRedraw();
      }
    }

    void height(int deltaHeight, osgGA::GUIActionAdapter& aa)
    {
      osg::View* view = aa.asView();
      if (view)
      {
        g_eyeAlt += 0.1*((double)deltaHeight);
        aa.requestRedraw();
      }
    }
  };

  simData::ObjectId createShip(simData::DataStore& dataStore)
  {
    simData::ObjectId result;

    // create the platform in the database:
    simData::DataStore::Transaction transaction;
    {
      simData::PlatformProperties* props = dataStore.addPlatform(&transaction);
      result = props->id();
      transaction.complete(&props);
    }

    // initialize the prefs:
    {
      simData::PlatformPrefs* prefs = dataStore.mutable_platformPrefs(result, &transaction);
      prefs->mutable_commonprefs()->set_name(PLATFORM_SHIP);
      prefs->set_icon(EXAMPLE_SHIP_ICON);
      prefs->set_dynamicscale(true);
      prefs->mutable_commonprefs()->mutable_labelprefs()->set_draw(false);
      transaction.complete(&prefs);
    }

    // give it a starting position:
    {
      // convert to ECEF:
      simCore::Coordinate ecef;
      simCore::CoordinateConverter::convertGeodeticToEcef(s_shipPosOri, ecef);

      simData::PlatformUpdate* newUpdate = dataStore.addPlatformUpdate(result, &transaction);
      newUpdate->set_x(ecef.x());
      newUpdate->set_y(ecef.y());
      newUpdate->set_z(ecef.z());
      newUpdate->set_psi(ecef.psi());
      newUpdate->set_theta(ecef.theta());
      newUpdate->set_phi(ecef.phi());
      newUpdate->set_time(0.0);
      transaction.complete(&newUpdate);
    }

    dataStore.update(0.0);

    return result;
  }

  /** Factory for a sky node */
  osgEarth::SkyNode* makeSky(osgEarth::MapNode* mapNode, bool useSilverLining, const std::string& slUser="", const std::string& slLicense="", const std::string& resourcePath="")
  {
#ifdef HAVE_OSGEARTH_SILVERLINING
    if (useSilverLining)
    {
      osgEarth::SilverLining::SilverLiningOptions sl;
      if (!slUser.empty())
        sl.user() = slUser;
      if (!slLicense.empty())
        sl.licenseCode() = slLicense;
      if (!resourcePath.empty())
        sl.resourcePath() = resourcePath;
      sl.drawClouds() = true;
      sl.cloudsMaxAltitude() = 100000.0;
      return new osgEarth::SilverLining::SilverLiningNode(sl);
    }
    else
#endif
    {
      return osgEarth::SkyNode::create();
    }
  }

}


int main(int argc, char** argv)
{
  simCore::checkVersionThrow();
  simExamples::configureSearchPaths();

  // Construct a map - this one uses all local data and was bathymetry
  // which is required for an ocean surface.
  osg::ref_ptr<osgEarth::Map> map = simExamples::createHawaiiMapLocalWithBathymetry();

  // check for ocean and sky options.
  osg::ArgumentParser ap(&argc, argv);

  // Read licensing arguments for SilverLining
  std::string sluser;
  std::string sllicense;
  std::string slpath = simExamples::getSilverLiningResourcesPath(); // SilverLining Resource Path
  osg::ArgumentParser::Parameter sluserArg(sluser);
  osg::ArgumentParser::Parameter sllicenseArg(sllicense);
  osg::ArgumentParser::Parameter slpathArg(slpath);
  bool useSilverLining = ap.read("--silverlining");
  ap.read("--sluser", sluserArg);
  ap.read("--sllicense", sllicenseArg);
  ap.read("--slpath", slpathArg);

  // Read licensing arguments for Triton
  std::string tritonUser;
  std::string tritonLicense;
  std::string tritonPath = simExamples::getTritonResourcesPath(); // Triton Resource Path
  osg::ArgumentParser::Parameter tritonUserArg(tritonUser);
  osg::ArgumentParser::Parameter tritonLicenseArg(tritonLicense);
  osg::ArgumentParser::Parameter tritonPathArg(tritonPath);
  bool useTriton = ap.read("--triton");
  ap.read("--tritonuser", tritonUserArg);
  ap.read("--tritonlicense", tritonLicenseArg);
  ap.read("--tritonpath", tritonPathArg);

  // Reticle overlay to use.
  osg::ref_ptr<osg::Image> reticle;
  std::string reticlePath;
  if (ap.read("--reticle", reticlePath))
  {
    reticle = osgDB::readImageFile(reticlePath);
  }
  else
  {
    reticle = osgDB::readImageFile("reticle.png");
  }

  // Alert user if we don't have a reticle
  if (!reticle.valid())
  {
    SIM_WARN << "No reticle image specified; please use --reticle <path>" << std::endl;
  }

  // use the reticle size as the viewport size.
  int width = 800, height = 600;
  if (reticle.valid())
  {
    width = reticle->s();
    height = reticle->t();
  }

  // start up a SIMDIS viewer
  osg::ref_ptr<simVis::Viewer> viewer = new simVis::Viewer();
  viewer->setMap(map.get());
  osg::ref_ptr<simVis::SceneManager> scene = viewer->getSceneManager();

  // the data store houses the entity data model:
  simData::MemoryDataStore dataStore;
  simVis::ScenarioDataStoreAdapter adapter(&dataStore, scene->getScenario());

  // create a ship to look at:
  s_shipId = createShip(dataStore);

  // add an ocean surface to the scene.
#ifdef HAVE_OSGEARTH_TRITON
  if (useTriton)
  {
    osgEarth::Triton::TritonLayer* layer = new osgEarth::Triton::TritonLayer();
    layer->setUserName(tritonUser);
    layer->setLicenseCode(tritonLicense);
    layer->setResourcePath(tritonPath);
    layer->setUseHeightMap(false);
    layer->setMaxAltitude(30000.0f);
    layer->setRenderBinNumber(simVis::BIN_OCEAN);
#if OSGEARTH_SOVERSION >= 154
    // render Triton on the ellipsoid (old default)
    layer->setVerticalDatum("");
#endif
    scene->getMap()->addLayer(layer);
  }
  else
#endif
  {
    osgEarth::SimpleOceanLayer* layer = new osgEarth::SimpleOceanLayer();
    scene->getMap()->addLayer(layer);
  }

  // Add a sky
  // add a sky to the scene.
  osg::ref_ptr<osgEarth::SkyNode> sky = makeSky(scene->getMapNode(), useSilverLining, sluser, sllicense, slpath);
  sky->attach(viewer->getMainView());
  sky->setDateTime(osgEarth::DateTime(2014, 4, 22, 16.5));
  scene->setSkyNode(sky.get());

  // remove the default manipulator; we will set the camera manually
  viewer->getMainView()->setUpViewInWindow(20, 20, width, height, 0);
  viewer->getMainView()->setCameraManipulator(nullptr);

  // apply the reticle overlay.
  simUtil::HudManager hudManager(viewer->getMainView(), viewer->getMainView()->getOrCreateHUD());
  if (reticle.valid())
    hudManager.createImage(reticle.get(), 0, 0, 100, 100);

  g_eyeLat = SUB_LAT;
  g_eyeLon = SUB_LON;
  g_eyeAlt = SUB_ALT;
  g_eyeAz  = 0.0;
  g_eyeEl  = 0.0;

  // install the handler for the demo keys in the notify() above
  viewer->addEventHandler(new ViewControls());

  viewer->installDebugHandlers();
  viewer->run();
}

