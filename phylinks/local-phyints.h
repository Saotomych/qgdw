// Data description for

u16 phy_handle;

// Structure consists data for route frames by path: multififo <-> socket
struct phy_route{
	u16 asdu;
	u16 mode;				// LISTEN || CONNECT
	char *name;				// example: device name kipp2m.iec104.tcp
	u32 mask;
	u32 ep_index;
	// sockets
	int socdesc;
	struct sockaddr_in 	sai;	// sai->sin_addr; sai->sin_port
	int state;
};
