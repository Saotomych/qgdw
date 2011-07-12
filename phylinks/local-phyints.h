// Data description for

u16 phy_handle;

#ifdef __unix__
typedef int	SOCKET;
#endif

// Structure consists data for route frames by path: multififo <-> socket
struct phy_route{
	u16 asdu;
	u16 mode;				// LISTEN || CONNECT
	char *name;				// example: device name kipp2m.phy_tcp
	u32 mask;
	u32 ep_index;
	int state;
	// sockets for phy_tcp
	SOCKET socdesc;
	struct sockaddr_in 	sai;		// sai->sin_addr->sadr; sai->sin_port
	struct sockaddr_in 	sailist;	// sai->sin_addr->sadr; sai->sin_port
	// symbol devices for phy_tty
	int realaddr;
	int devindex;
};
