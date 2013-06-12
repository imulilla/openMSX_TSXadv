#include "SuperImposedVideoFrame.hh"
#include "LineScalers.hh"
#include "vla.hh"
#include "build-info.hh"
#include <cstdint>

namespace openmsx {

template <typename Pixel>
SuperImposedVideoFrame<Pixel>::SuperImposedVideoFrame(
		const FrameSource& src_, const FrameSource& super_,
		const PixelOperations<Pixel>& pixelOps_)
	: FrameSource(pixelOps_.getSDLPixelFormat())
	, src(src_), super(super_), pixelOps(pixelOps_)
{
	setHeight(src.getHeight());
}

template <typename Pixel>
unsigned SuperImposedVideoFrame<Pixel>::getLineWidth(unsigned line) const
{
	unsigned width = src.getLineWidth(line);
	return (width == 1) ? 320 : width;
}

template <typename Pixel>
const void* SuperImposedVideoFrame<Pixel>::getLineInfo(unsigned line, unsigned& width) const
{
	// Return minimum line width of 320.
	//  We could check whether both inputs have width=1 and in that case
	//  also return a line of width=1. But for now (laserdisc) this will
	//  never happen.
	auto* srcLine = static_cast<const Pixel*>(src.getLineInfo(line, width));
	if (width == 1) {
		width = 320;
		srcLine = src.getLinePtr<Pixel>(line, 320);
	}

	// Adjust the two inputs to the same height.
	const Pixel* supLine;
	VLA_SSE_ALIGNED(Pixel, tmpBuf, width);
	assert(super.getHeight() == 480); // TODO possibly extend in the future
	if (src.getHeight() == 240) {
		auto* sup0 = super.getLinePtr<Pixel>(2 * line + 0, width);
		auto* sup1 = super.getLinePtr<Pixel>(2 * line + 1, width);
		BlendLines<Pixel, 1, 1> blend(pixelOps);
		blend(sup0, sup1, tmpBuf, width);
		supLine = tmpBuf;
	} else {
		assert(src.getHeight() == super.getHeight());
		supLine = super.getLinePtr<Pixel>(line, width); // scale line
	}

	// Actually blend the lines of both frames.
	auto* resLine = static_cast<Pixel*>(src.getTempBuffer());
	AlphaBlendLines<Pixel> blend(pixelOps);
	blend(srcLine, supLine, resLine, width);
	return resLine;
}

// Force template instantiation.
#if HAVE_16BPP
template class SuperImposedVideoFrame<uint16_t>;
#endif
#if HAVE_32BPP
template class SuperImposedVideoFrame<uint32_t>;
#endif

} // namespace openmsx
