// Data description for

u16 phy_handle;

// Structure consists data for route frames by path: multififo <-> socket
struct phy_route{
	char *name;
	u32 mask;
	u16 mode;
	u32 ep_index;
	// sockets
	int socdesc;
	struct sockaddr_in sai;	// sai->sin_addr; sai->sin_port
	int state;
};
