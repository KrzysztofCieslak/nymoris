use limine::request::{ExecutableAddressRequest, FramebufferRequest, MemmapRequest};
use limine::{BaseRevision, RequestsStartMarker, RequestsEndMarker};

#[used]
#[link_section = ".limine_requests_start"]
static START_MARKER: RequestsStartMarker = RequestsStartMarker::new();

#[used]
#[link_section = ".limine_requests"]
static BASE_REVISION: BaseRevision = BaseRevision::with_revision(0);

#[used]
#[link_section = ".limine_requests"]
pub static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

#[used]
#[link_section = ".limine_requests"]
pub static MEMMAP_REQUEST: MemmapRequest = MemmapRequest::new();

#[used]
#[link_section = ".limine_requests"]
pub static EXEC_ADDRESS_REQUEST: ExecutableAddressRequest = ExecutableAddressRequest::new();

#[used]
#[link_section = ".limine_requests_end"]
static END_MARKER: RequestsEndMarker = RequestsEndMarker::new();
