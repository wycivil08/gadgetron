import numpy as np
import GadgetronPythonMRI as g
import GadgetronXML
import kspaceandimage as ki

myLocalGadgetReference = g.GadgetReference()
myBuffer = 0
myParameters = 0

def set_gadget_reference(gadref):
    global myLocalGadgetReference
    myLocalGadgetReference = gadref

def config_function(conf):
    global myBuffer
    global myParameters
    myParameters =  GadgetronXML.getEncodingParameters(conf)
    myBuffer = (np.zeros((myParameters["channels"],myParameters["slices"],myParameters["matrix_z"],myParameters["matrix_y"],myParameters["matrix_x"]))).astype('complex64')

def recon_function(acq, data):
    global myLocalGadgetReference
    global myBuffer
    global myParameters

    line_offset = (myParameters["matrix_y"]-myParameters["phase_encoding_lines"])>>1;
    myBuffer[:,acq.idx.slice,acq.idx.partition,acq.idx.line+line_offset,:] = data
    
    if (acq.flags & (1<<1)): #Is this the last scan in slice
        image = ki.ktoi(myBuffer,(2,3,4))
        image = image * np.product(image.shape) #Scaling for the scanner
        #Create a new image header and transfer value
        img_head = g.GadgetMessageImage()
        img_head.channels = acq.channels
        img_head.data_idx_curent = acq.idx
        img_head.data_idx_max = acq.max_idx
        img_head.data_idx_min = acq.min_idx
        img_head.set_matrix_size(0,myBuffer.shape[4])
        img_head.set_matrix_size(1,myBuffer.shape[3])
        img_head.set_matrix_size(2,myBuffer.shape[2])
        img_head.set_matrix_size(3,myBuffer.shape[1])
        img_head.set_position(0,acq.get_position(0))
        img_head.set_position(1,acq.get_position(1))
        img_head.set_position(2,acq.get_position(2))
        img_head.set_quarternion(0,acq.get_quarternion(0))
        img_head.set_quarternion(1,acq.get_quarternion(1))
        img_head.set_quarternion(2,acq.get_quarternion(2))
        img_head.set_quarternion(3,acq.get_quarternion(3))
        img_head.time_stamp = acq.time_stamp
        #Return image to Gadgetron
        myLocalGadgetReference.return_image(img_head,image.astype('complex64'))

