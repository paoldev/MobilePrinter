// <CDATA>
=== Index ====================================================================
* Quick start guide
  + Service
  + Compiling
* Generated files
* Generated interfaces
  + Base interfaces
  + Event notify interfaces
* Generated classes
  + Event source classes
* Generated functions
  + Host builder function
  + Stub functions
* Generated structures
  + Metadata structures
  + Parameter structures




=== Quick start guide ========================================================
--- Service --------------------------
To build a service, implement one or more of the following
interfaces:
* IPrinterServiceType
* IPrinterServiceV12Type
* IPrinterServiceV20Type

Then, pass one or more of these objects into the host builder function,
These interfaces do NOT have to be implemented by the same object.
CreateMyPrinterHost() (see below).  Lastly, call Start()
on the resulting IWSDDeviceHost object.



--- Compiling -----------------------
All generated files must be compiled together into one static library,
executable, or DLL.

The generated IDL file (MyPrinter.idl) may be compiled into
C++ files with the MIDL tool.  The other generated files require that
MyPrinter.idl will be compiled into MyPrinter.h.




=== Generated files ==========================================================
* MyPrinterTypes.h    Forward-declarations and struct definitions
* MyPrinterTypes.cpp  Type table and operation structure definitions
* MyPrinter.idl       Defines the interfaces for the specified services
* MyPrinterProxy.h    Declares proxy classes and builder functions
* MyPrinterProxy.cpp  Proxy class and function implementations
* MyPrinterStub.cpp   Stub function code



=== Generated interfaces =====================================================
--- Base interfaces ------------------
* IPrinterServiceType
* IPrinterServiceV12Type
* IPrinterServiceV20Type

These base interfaces are generated directly by the WSDL, and are used to
implement your service.  Clients should use the extended proxy classes,
below.  No eventing operations are included in these base interfaces.

Your service objects should implement these interfaces.



--- Event notify interfaces ----------
* IPrinterServiceTypeEventNotify
* IPrinterServiceV20TypeEventNotify

These event notify interfaces are used by services to issue events, and are
implemented by client code to receive events.  Subscription management is
handled separately in the proxy interfaces (listed above).

Your service code should instantiate objects that expose these interfaces
using the event source builder functions (see below), and should call into
the exposed methods.

You should build a client object that implements these interfaces, and
register that object when subscribing for events.  Your object will receive
callbacks when you receive events from the service.



=== Generated classes ========================================================
--- Event source classes -------------
* CPrinterServiceTypeEventSource
* CPrinterServiceV20TypeEventSource

These event source classes can be called from your service, and will issue
events to subscribed clients.  To instantiate one of these classes, use a
event source builder function (see below).




=== Generated functions ======================================================
--- Host builder function ------------
* CreateMyPrinterHost()

Use this function to create a host and register your service(s).


--- Event source builder functions ---
* CreateCPrinterServiceTypeEventSource()
* CreateCPrinterServiceV12TypeEventSource()
* CreateCPrinterServiceV20TypeEventSource()

Use these functions to generate event source classes (see above).



--- Stub functions -------------------
These functions receive calls from WSDAPI and dispatch them into your
service object.  You should not call these functions from your application.




=== Generated structures =====================================================
--- Metadata structures --------------
* WSD_HOST_METADATA hostMetadata
* WSD_THIS_MODEL_METADATA thisModelMetadata

Pass these structures to your host builder function (see above).



--- Parameter structures -------------
All parameter structures are defined inside MyPrinterTypes.h.

// </CDATA>

