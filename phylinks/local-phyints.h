// Data description for

u16 phy_handle;

#ifdef __unix__
typedef int	SOCKET;
#endif

// Structure consists data for route frames by path: multififo <-> socket
struct phy_route{
	u16 asdu;
	u16 mode;				// LISTEN || CONNECT
	char *name;				// example: device name kipp2m.iec104.tcp
	u32 mask;
	u32 ep_index;
	// sockets
	SOCKET socdesc;
	struct sockaddr_in 	sai;		// sai->sin_addr; sai->sin_port
	struct sockaddr_in 	sailist;	// sai->sin_addr; sai->sin_port
	int state;
	// symbol device
	int fdesc;
	int devindex;
};
