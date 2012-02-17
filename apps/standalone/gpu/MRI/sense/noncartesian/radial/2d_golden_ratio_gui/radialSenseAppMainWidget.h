#pragma once

// Autogenerated header by uic
#ifdef _WIN32
#include "radialSenseAppBaseMainWidget.ui.h"
#else
#include "ui_radialSenseAppBaseMainWidget.h" 
#endif 

// Gadgetron includes
#include "vector_td.h"
#include "hoNDArray.h"
#include "cuNDArray.h"
#include "NFFT.h"
#include "cuCGSolver.h"
#include "cuNonCartesianSenseOperator.h"
#include "cuImageOperator.h"
#include "cuCGPrecondWeights.h"
#include "complext.h"

#include <boost/smart_ptr.hpp>

class radialSenseAppMainWindow : public QMainWindow, public Ui::radialSenseAppBaseMainWindow
{
  // Macro for the Qt gui
  Q_OBJECT
 
  public:

  // Constructor
  radialSenseAppMainWindow(QWidget *parent = 0);

  // Reconstruct frame
  void reconstruct();

  // Get matrix size
  inline uintd2::Type get_matrix_size();

  // Get oversampled matrix size
  inline uintd2::Type get_matrix_size_os();

  // Get number of coils
  inline unsigned int get_num_coils();

  // Get kernel width
  inline float get_kernel_width();

  // Get kappa (regularization weight)
  inline float get_kappa();

  // Get first projection
  inline unsigned int get_first_projection();

  // Get central projection
  inline unsigned int get_central_projection();

  // Get maximum central projection
  inline unsigned int get_maximum_central_projection();

  // Get number of projections per frame
  inline unsigned int get_num_projections_per_frame();

  // Number of samples per projection
  inline unsigned int get_num_samples_per_projection();

  // Number of points per reconstruction
  inline unsigned int get_num_points_per_reconstruction();

  // Get host side sample data array
  inline hoNDArray<complext<float> >* get_sample_values_array();

  // Get number of points per coil in data array
  unsigned int get_num_points_per_array_coil();

  // Get number of iterations
  unsigned int get_num_iterations();

  // Get window scale
  float get_window_scale();
  
  boost::shared_ptr< cuNDArray<float_complext> >
  upload_data( unsigned int profile_offset, unsigned int samples_per_profile, unsigned int samples_per_reconstruction, 
	       unsigned int total_samples_per_coil, unsigned int num_coils, hoNDArray<float_complext> *host_data );

private:
  void resetPrivateData();
  void replan();
  void update_preconditioning_weights();	       

private slots:
  void open();
  void close();
  void saveImage();
  void matrixSizeChanged();
  void matrixSizeOSChanged();
  void regularizationWeightChanged();
  void projectionsPerFrameChanged(int);
  void centralProjectionChanged(int);
  void numIterationsChanged();
  void kernelWidthChanged();
  void windowScaleChanged(double);

private:
	
  // Reconstruction plan
  NFFT_plan<float,2> plan;

  // Define conjugate gradient solver
  cuCGSolver<float, float_complext> cg;

  // Define non-Cartesian Sense solver
  boost::shared_ptr< cuNonCartesianSenseOperator<float,2> > E;

  // Define preconditioner
  boost::shared_ptr< cuCGPrecondWeights<float_complext> > D;
  
  // Define regularization image operator
  boost::shared_ptr< cuImageOperator<float,float_complext> > R;
  
  // CSM
  boost::shared_ptr< cuNDArray<float_complext> > csm;

  // Density compensation weights
  boost::shared_ptr< cuNDArray<float> > dcw;	

  // Host data array
  boost::shared_ptr< hoNDArray<float_complext> > host_samples;
  
  // Label for the status bar
  QLabel *statusLabel;

  // Are we set up for reconstruction?
  bool ready;
};

