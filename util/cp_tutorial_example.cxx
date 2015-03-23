// This is a skeleton C++ executable code for the CP tools tutorial session
// of the ATLAS Run-2 performance kickstart meeting at LBNL.
// author: Steve Farrell <Steven.Farrell@cern.ch>

// ROOT includes
#include "TFile.h"
#include "TError.h"

// Infrastructure includes
#include "xAODRootAccess/Init.h"
#include "xAODRootAccess/TEvent.h"
#include "xAODRootAccess/TStore.h"
#include "xAODCore/ShallowCopy.h"
#include "AsgTools/StatusCode.h"

// EDM includes
#include "xAODEventInfo/EventInfo.h"
#include "xAODMuon/MuonContainer.h"

// CP includes
#include "MuonMomentumCorrections/MuonCalibrationAndSmearingTool.h"

// Error checking macro
#define CHECK( ARG )                                 \
  do {                                               \
    const bool result = ARG;                         \
    if(!result) {                                    \
      ::Error(APP_NAME, "Failed to execute: \"%s\"", \
              #ARG );                                \
      return 1;                                      \
    }                                                \
  } while( false )

// Our main function
int main(int argc, char* argv[])
{
  // The application's name
  const char* APP_NAME = argv[0];

  // Check if we received a file name
  if(argc < 2) {
    Error( APP_NAME, "No file name received!" );
    Error( APP_NAME, "  Usage: %s [xAOD file name]", APP_NAME );
    return 1;
  }

  // Initialise the application
  CHECK( xAOD::Init(APP_NAME) );
  StatusCode::enableFailure();

  // Open the input file
  const char* fileName = argv[1];
  Info(APP_NAME, "Opening file: %s", fileName);
  std::unique_ptr<TFile> file(TFile::Open(fileName));
  CHECK( file.get() );

  // Create a TEvent object
  xAOD::TEvent event;
  CHECK( event.readFrom(file.get()) );
  Info(APP_NAME, "Number of events in the file: %lli", event.getEntries());

  // Create a transient store
  xAOD::TStore store;



  // @@@ Create and configure your CP tools here @@@ //

  // Muon calib tool
  CP::MuonCalibrationAndSmearingTool muonCalibTool("MuonCalibrationAndSmearingTool");
  CHECK( muonCalibTool.initialize() );

  // Loop over events
  Long64_t nEntries = event.getEntries();
  nEntries = std::min(nEntries, 20ll); // Remove this to run all events
  for(Long64_t entry = 0; entry < nEntries; ++entry)
  {
    // Tell TEvent which entry to use
    event.getEntry(entry);

    // Retrieve and print basic event information
    const xAOD::EventInfo* evtInfo = 0;
    CHECK( event.retrieve(evtInfo, "EventInfo") );
    Info(APP_NAME,
         "===>>> Processing event #%lli, "
         "run #%u, %lli events processed so far  <<<===",
         evtInfo->eventNumber(), evtInfo->runNumber(), entry);

    // Retrieve the muon container
    const xAOD::MuonContainer* muons = 0;
    CHECK( event.retrieve(muons, "Muons") );

    // Print the number of muons
    Info(APP_NAME, "Number of muons: %lu", muons->size());

    // Create a shallow copy of the muons
    std::pair<xAOD::MuonContainer*, xAOD::ShallowAuxContainer*> muonCopyPair =
      xAOD::shallowCopyContainer(*muons);

    // Calibrate my muons
    xAOD::MuonContainer* myMuons = muonCopyPair.first;
    for(auto muon : *myMuons){
      CHECK( muonCalibTool.applyCorrection(*muon) != CP::CorrectionCode::Error );
    }

    // Record our copies in the transient store
    CHECK( store.record(muonCopyPair.first, "MyMuons") );
    CHECK( store.record(muonCopyPair.second, "MyMuonsAux.") );

    // Clear the transient store
    store.clear();
  }

  // Closing message
  Info(APP_NAME, "Application finished");

  return EXIT_SUCCESS;
}
