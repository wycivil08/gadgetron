#include "CloudBus.h"
#include "log.h"

namespace Gadgetron
{
  CloudBus* CloudBus::instance_ = 0;
  const char* CloudBus::relay_inet_addr_ = GADGETRON_DEFAULT_RELAY_ADDR;
  int CloudBus::relay_port_ = GADGETRON_DEFAULT_RELAY_PORT;
  bool CloudBus::query_mode_ = false; //Listen only is disabled default
  int CloudBus::gadgetron_port_ = 9002; //Default port

  CloudBus* CloudBus::instance()
  {
    if (!instance_)
      {
	instance_ = new CloudBus(relay_port_, relay_inet_addr_);
	instance_->open();
      }
    return instance_;
  }

  
  void CloudBus::set_relay_address(const char* addr)
  {
    relay_inet_addr_ = addr;
  }

  void CloudBus::set_relay_port(int port)
  {
    relay_port_ = port;
  }

  void CloudBus::set_query_only(bool m)
  {
    query_mode_ = m;
  }
 
  void CloudBus::set_gadgetron_port(uint32_t port)
  {
    gadgetron_port_ = port;
  }

  void CloudBus::set_compute_capability(uint32_t c)
  {
    node_info_.compute_capability = c;
  }


  int CloudBus::open(void*)
  {
    if (!this->reactor()) {
      this->reactor(ACE_Reactor::instance());
    }
    return this->activate( THR_NEW_LWP | THR_JOINABLE,1); //single thread
  }

  

  int CloudBus::handle_close (ACE_HANDLE handle, ACE_Reactor_Mask close_mask)
  {
    GDEBUG("Cloud bus connection closed\n");
    this->peer().close();
    connected_ = false;
    return 0;
  }

  int CloudBus::handle_input (ACE_HANDLE fd)
  {
    size_t recv_cnt;
    uint32_t msg_size;
    if ((recv_cnt = peer().recv_n (&msg_size, sizeof(uint32_t))) <= 0) {
      GDEBUG("Failed to get msg_size from relay. Relay must have disconnected\n");
      return -1;
    }
    char* buffer = new char[msg_size];
    nodes_.clear();
    if ((recv_cnt = peer().recv_n (buffer, msg_size)) <= 0) {
      GDEBUG("Failed to read message from relay. Relay must have disconnected\n");
      delete [] buffer;
      return -1;
    }
      
    uint32_t msg_id = *((uint32_t*)buffer);
    if (msg_id != GADGETRON_CLOUDBUS_NODE_LIST_REPLY) {
      GERROR("Unexpected message id = %d\n", msg_id);
      return -1;
    }
    deserialize(nodes_, buffer+4, msg_size-4);
    delete [] buffer;
    node_list_condition_.signal();
    return 0;
  }

  
  int CloudBus::svc(void)
  {
    
    while (true) {
      if (connected_) {
	//
      } else {
	std::string connect_addr(relay_inet_addr_);
	if (connect_addr == "localhost") {
	  connect_addr = node_info_.address;
	}
	ACE_INET_Addr server(relay_port_,connect_addr.c_str());
	ACE_SOCK_Connector connector;

	if (connector.connect(this->peer(),server) == 0) {
	  ACE_TCHAR peer_name[MAXHOSTNAMELENGTH];
	  ACE_INET_Addr peer_addr;
	  if ((this->peer().get_remote_addr (peer_addr) == 0) && 
	      (peer_addr.addr_to_string (peer_name, MAXHOSTNAMELENGTH) == 0)) {

	    GDEBUG("CloudBus connected to relay at  %s\n", peer_name);
	    if (this->reactor ()->register_handler(this, ACE_Event_Handler::READ_MASK) != 0) {
	      GERROR("Failed to register read handler\n");
	      return -1;
	    }

	    mtx_.acquire();
	    connected_ = true;
	    mtx_.release();
	    if (!query_mode_) {
	      send_node_info();
	    }
	  } 
	}
      }
      //Sleep for 5 seconds
      ACE_Time_Value tv (5);
      ACE_OS::sleep (tv);	  	
    }
    return 0;
  }

  void CloudBus::send_node_info()
  {
    size_t buf_len = calculate_node_info_length(node_info_);
    try {
      char* buffer = new char[4+4+buf_len];
      *((uint32_t*)buffer) = buf_len+4;
      *((uint32_t*)(buffer + 4)) = GADGETRON_CLOUDBUS_NODE_INFO;
      if (connected_) {
	serialize(node_info_,buffer + 8,buf_len);
	this->peer().send_n(buffer,buf_len+8);
      }
      delete [] buffer;
    } catch (...) {
      GERROR("Failed to send gadgetron node info\n");
      throw;
    }
    print_nodes();
  }

  void CloudBus::update_node_info()
  {
    mtx_.acquire();
    if (connected_) {
      uint32_t req[2];
      req[0] = 4;
      req[1] = GADGETRON_CLOUDBUS_NODE_LIST_QUERY;

      this->peer().send_n((char*)(&req),8);
      node_list_condition_.wait();
    }
    mtx_.release();
  }
  
  void CloudBus::print_nodes()
  {
    std::vector<GadgetronNodeInfo> nl;
    get_node_info(nl);
    GDEBUG("Number of available nodes: %d\n", nl.size());
    for (std::vector<GadgetronNodeInfo>::iterator it = nl.begin();
	 it != nl.end(); it++)
      {
	GDEBUG("  %s, %s, %d\n", it->uuid.c_str(), it->address.c_str(), it->port);
      }
  }
  
  void CloudBus::get_node_info(std::vector<GadgetronNodeInfo>& nodes)
  {
    update_node_info();
    mtx_.acquire();
    nodes = nodes_;
    mtx_.release();
  }
  
  size_t CloudBus::get_number_of_nodes()
  {
    update_node_info();
    size_t nodes;
    mtx_.acquire();
    nodes = nodes_.size();
    mtx_.release();
    return nodes;
  }

  CloudBus::CloudBus(int port, const char* addr)
    : mtx_("CLOUDBUSMTX")
    , mtx_node_list_("CLOUDBUSMTXNODELIST")
    , node_list_condition_(mtx_node_list_)
    , uuid_(boost::uuids::random_generator()())
    , connected_(false)      
  {
    node_info_.port = gadgetron_port_;
    set_compute_capability(1);
    node_info_.uuid = boost::uuids::to_string(uuid_);
    ACE_SOCK_Acceptor listener (ACE_Addr::sap_any);
    ACE_INET_Addr local_addr;
    listener.get_local_addr (local_addr);
    node_info_.address = std::string(local_addr.get_host_name());
  }

}
