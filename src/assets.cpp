#include "platform.h"
#include "util.h"
#include "assets.h"

#include <wincodec.h>
#include <dwrite.h>
#include <d2d1.h>

using namespace std;

namespace {

	bool file_exists(std::string const& path)
	{
		auto const attribs = GetFileAttributes(to_utf16(path).c_str());
		return (attribs != INVALID_FILE_ATTRIBUTES) &&
			((attribs & FILE_ATTRIBUTE_DIRECTORY) == 0);
	}

	//
	// creates a full path to a file under <USER>\AppData\Local\d3d-9211
	//
	string get_temp_filename(std::string const& filename)
	{
		PWSTR wpath = nullptr;
		if (SUCCEEDED(
			SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, 0, &wpath)))
		{
			wstring utf16_path(wpath);
			CoTaskMemFree(wpath);

			utf16_path.append(L"\\");
			utf16_path.append(L"d3d-9211");
			CreateDirectory(utf16_path.c_str(), 0);
			utf16_path.append(L"\\");
			utf16_path.append(to_utf16(filename));
			return to_utf8(utf16_path);
		}

		assert(0);
		return "";
	}

	class Image : public IImage
	{
	private:
		shared_ptr<void> const buffer_;
		uint32_t const width_;
		uint32_t const height_;
		uint32_t const stride_;

	public:
		Image(shared_ptr<void> const& buffer, 
			uint32_t width, uint32_t height, uint32_t stride)
			: buffer_(buffer)
			, width_(width)
			, height_(height)
			, stride_(stride) {
		}
		
		uint32_t width() const { return width_; }
		uint32_t height() const { return height_; }

		void* lock(uint32_t& stride) {
			stride = stride_;
			return buffer_.get();
		}

		void unlock() {
		}
	};


	class Assets : public IAssets
	{
	private:
		shared_ptr<IWICImagingFactory> wic_;
		shared_ptr<ID2D1Factory> d2d_;
		shared_ptr<IDWriteFactory> dwrite_;
		
	public:

		Assets(IWICImagingFactory* wic, 
			ID2D1Factory* d2d, 
			IDWriteFactory* dwrite)
			: wic_(to_com_ptr(wic))
			, d2d_(to_com_ptr(d2d))
			, dwrite_(to_com_ptr(dwrite))
		{
		
		}
		
		void generate(uint32_t width, uint32_t height) override
		{
			auto canvas = generate_transparent(16, 16);
			if (canvas) {
				save_canvas(canvas, get_temp_filename("transparent.png"));
			}

			canvas = generate_meter("Direct3D9", width, height);
			if (canvas) {
				save_canvas(canvas, get_temp_filename("d3d9_bg.png"));
			}

		}

		shared_ptr<string> locate(std::string const& filename) override
		{
			auto const path = get_temp_filename(filename);
			if (file_exists(path)) {
				return make_shared<string>(path);
			}
			return nullptr;
		}

		shared_ptr<IImage> load_image(
			shared_ptr<std::string> const& filename) override
		{
			if (!filename) {
				return nullptr;
			}

			auto utf16 = to_utf16(*filename);

			IWICBitmapDecoder* pdec = nullptr;
			auto hr = wic_->CreateDecoderFromFilename(
				utf16.c_str(),
				nullptr,
				GENERIC_READ,
				WICDecodeMetadataCacheOnDemand,
				&pdec);
			if (FAILED(hr)) {
				return nullptr;
			}

			auto const decoder = to_com_ptr(pdec);

			IWICBitmapFrameDecode* pfrm = nullptr;
			hr = decoder->GetFrame(0, &pfrm);
			if (FAILED(hr)) {
				return nullptr;
			}

			auto const frame = to_com_ptr(pfrm);

			IWICFormatConverter* pcnv = nullptr;
			hr = wic_->CreateFormatConverter(&pcnv);
			if (FAILED(hr)) {
				return nullptr;
			}
			auto const converter = to_com_ptr(pcnv);

			hr = converter->Initialize(
				frame.get(), GUID_WICPixelFormat32bppPBGRA,
				WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);
			if (FAILED(hr)) {
				return nullptr;
			}

			UINT width, height;
			frame->GetSize(&width, &height);

			uint32_t stride = width * 4;
			uint32_t cb = height * stride;

			shared_ptr<BYTE> buffer(
				reinterpret_cast<BYTE*>(malloc(cb)), free);

			hr = converter->CopyPixels(nullptr, stride, cb, buffer.get());
			if (SUCCEEDED(hr)) {
				return make_shared<Image>(buffer, width, height, stride);
			}
			return nullptr;
		}

		//
		// draw a simple checker-board image for showing transparency
		//
		shared_ptr<IWICBitmap> generate_transparent(uint32_t width, uint32_t height)
		{
			auto const canvas = create_canvas(width, height);
			auto const ctx = create_context(canvas);
			if (!ctx) {
				return nullptr;
			}

			ctx->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

			ctx->BeginDraw();
			ctx->SetTransform(D2D1::IdentityMatrix());
			ctx->Clear(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));

			auto const brush = create_solid_brush(ctx, D2D1::ColorF(0.8f, 0.8f, 0.8f, 1.0f));

			D2D1_RECT_F box;

			box.left = width / 2.0f;
			box.top = 0.0f;
			box.right = box.left + (width / 2.0f);
			box.bottom = box.top + (height / 2.0f);
			ctx->FillRectangle(box, brush.get());

			box.left = 0.0f;
			box.top = height / 2.0f;
			box.right = box.left + (width / 2.0f);
			box.bottom = box.top + (height / 2.0f);
			ctx->FillRectangle(box, brush.get());

			ctx->EndDraw();
			return canvas;
		}

		shared_ptr<IWICBitmap> generate_meter(string const& title, uint32_t width, uint32_t height)
		{
			auto const canvas = create_canvas(width, height);
			auto const ctx = create_context(canvas);
			if (!ctx) {
				return nullptr;
			}
			
			ctx->BeginDraw();
			ctx->SetTransform(D2D1::IdentityMatrix());
			ctx->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));

			auto const white_brush = create_solid_brush(
				ctx, D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
			auto const red_brush = create_solid_brush(
				ctx, D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f));
			auto const yellow_brush = create_solid_brush(
				ctx, D2D1::ColorF(1.0f, 0.95f, 0.0f, 1.0f));
			auto const green_brush = create_solid_brush(
				ctx, D2D1::ColorF(0.13f, 0.69f, 0.30f, 1.0f));

			float size = height * 0.85f;
			float stroke = 16.0;

			auto const radius = to_dips(ctx, size / 2, size / 2);
			auto const center = to_dips(ctx, width / 2.0f, height / 2.0f);

			// draw arcs for highlights
			auto path = create_path();
			if (path)
			{
				auto const r = radius.x + stroke - 1.0f;

				draw_arc(ctx, red_brush, center, r, 25.0f, -45.0f, stroke);
				draw_arc(ctx, red_brush, center, r, 25.0f, 20.0f, stroke);
				draw_arc(ctx, yellow_brush, center, r, 10.0f, -20.0f, stroke);
				draw_arc(ctx, yellow_brush, center, r, 10.0f, 10.0f, stroke);
				draw_arc(ctx, green_brush, center, r, 20.0f, -10.0f, stroke);
			}

			{ // draw tic marks

				auto const format = create_text_format("Lucida Console", radius.y * 0.08f);

				auto const width = stroke * 0.75f;
				auto const len = width * 4;

				draw_tic(ctx, white_brush, format, "45\xc2\xb0", center, radius.x, len, -45.0f, width);
				draw_tic(ctx, white_brush, format, "30\xc2\xb0", center, radius.x, len, -30.0f, width);
				draw_tic(ctx, white_brush, format, "20\xc2\xb0", center, radius.x, len, -20.0f, width);
				draw_tic(ctx, white_brush, format, "10\xc2\xb0", center, radius.x, len, -10.0f, width);
				draw_tic(ctx, white_brush, format, "0\xc2\xb0", center, radius.x, len, 0.0f, width);
				draw_tic(ctx, white_brush, format, "-10\xc2\xb0", center, radius.x, len, 10.0f, width);
				draw_tic(ctx, white_brush, format, "-20\xc2\xb0", center, radius.x, len, 20.0f, width);
				draw_tic(ctx, white_brush, format, "-30\xc2\xb0", center, radius.x, len, 30.0f, width);
				draw_tic(ctx, white_brush, format, "-45\xc2\xb0", center, radius.x, len, 45.0f, width);
			}

			{ // draw outer circle

				D2D1_ELLIPSE ellipse;
				ellipse.point.x = center.x;
				ellipse.point.y = center.y;
				ellipse.radiusX = radius.x;
				ellipse.radiusY = radius.y;

				ctx->SetTransform(D2D1::Matrix3x2F::Identity());

				ctx->DrawEllipse(&ellipse, white_brush.get(), stroke);
			}

			ctx->EndDraw();
			return canvas;
		}

		shared_ptr<IWICBitmap> create_canvas(uint32_t width, uint32_t height)
		{
			IWICBitmap* canvas = nullptr;
			auto const hr = wic_->CreateBitmap(width, height,
				GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &canvas);
			if (SUCCEEDED(hr)) {
				return to_com_ptr(canvas);
			}
			return nullptr;
		}
		
		bool save_canvas(shared_ptr<IWICBitmap> const& canvas, string const& path)
		{
			if (!canvas) {
				return false;
			}

			auto const stream = create_write_stream(path);
			if (!stream) {
				return false;
			}
			auto const encoder = create_encoder(GUID_ContainerFormatPng);
			if (!encoder) {
				return false;
			}

			auto hr = encoder->Initialize(stream.get(), WICBitmapEncoderNoCache);
			if (FAILED(hr)) {
				return false;
			}
			
			IWICBitmapFrameEncode* frame = nullptr;
			hr = encoder->CreateNewFrame(&frame, nullptr);
			if (FAILED(hr)) {
				return false;
			}

			bool success = false;
			if (SUCCEEDED(frame->Initialize(nullptr))) 
			{
				uint32_t width, height;
				canvas->GetSize(&width, &height);
				frame->SetSize(width, height);		

				WICPixelFormatGUID format = GUID_WICPixelFormatDontCare;
				hr = frame->SetPixelFormat(&format);

				hr = frame->WriteSource(canvas.get(), nullptr);
				hr = frame->Commit();
				
				if (SUCCEEDED(encoder->Commit())) {
					success = true;
				}
			}

			frame->Release();
			return success;
		}
		
		shared_ptr<ID2D1RenderTarget> create_context(shared_ptr<IWICBitmap> const& canvas)
		{
			if (!canvas) {
				return nullptr;
			}

			D2D1_RENDER_TARGET_PROPERTIES props = {};
			props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
			props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
			
			ID2D1RenderTarget* rt = nullptr;
			auto const hr = d2d_->CreateWicBitmapRenderTarget(canvas.get(), &props, &rt);
			if (SUCCEEDED(hr)) 
			{
				rt->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
				rt->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
				return to_com_ptr(rt);
			}
			return nullptr;
		}

		D2D_VECTOR_2F to_dips(shared_ptr<ID2D1RenderTarget> const& ctx, float x, float y)
		{
			D2D_VECTOR_2F v = { x, y };
			if (ctx) 
			{
				float dpi_x, dpi_y;
				ctx->GetDpi(&dpi_x, &dpi_y);
				v.x = (x * 96.0f) / dpi_x;
				v.y = (y * 96.0f) / dpi_y;
			}
			return v;
		}

		shared_ptr<ID2D1PathGeometry> create_path()
		{
			ID2D1PathGeometry* path = nullptr;
			auto const hr = d2d_->CreatePathGeometry(&path);
			if (SUCCEEDED(hr)) {
				return to_com_ptr(path);
			}
			return nullptr;
		}

		shared_ptr<ID2D1Brush> create_solid_brush(
			shared_ptr<ID2D1RenderTarget> const& ctx, D2D1_COLOR_F const& color)
		{
			if (ctx)
			{
				ID2D1SolidColorBrush* brush = nullptr;
				auto const hr = ctx->CreateSolidColorBrush(color, &brush);
				if (SUCCEEDED(hr)) {
					return to_com_ptr(brush);
				}			
			}
			return nullptr;
		}	

		shared_ptr<IDWriteTextFormat> create_text_format(string const& family, float size)
		{
			IDWriteTextFormat* format = nullptr;
			auto const hr = dwrite_->CreateTextFormat(
				to_utf16(family).c_str(),
				nullptr,
				DWRITE_FONT_WEIGHT_REGULAR,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				size,
				L"en-us",
				&format);
			if (SUCCEEDED(hr)) {
				return to_com_ptr(format);
			}
			return nullptr;
		}

		shared_ptr<IWICStream> create_write_stream(string const& path)
		{
			IWICStream* stream = nullptr;
			auto hr = wic_->CreateStream(&stream);
			if (SUCCEEDED(hr))
			{
				hr = stream->InitializeFromFilename(to_utf16(path).c_str(), GENERIC_WRITE);
				if (SUCCEEDED(hr)) {
					return to_com_ptr(stream);
				}
				stream->Release();
			}
			return nullptr;
		}

		shared_ptr<IWICBitmapEncoder> create_encoder(GUID const& id)
		{
			IWICBitmapEncoder* encoder = nullptr;
			auto const hr = wic_->CreateEncoder(id, nullptr, &encoder);
			if (SUCCEEDED(hr)) {
				return to_com_ptr(encoder);
			}
			return nullptr;
		}

		DWRITE_TEXT_METRICS measure(shared_ptr<IDWriteTextFormat> const& text_format, string const& label)
		{
			DWRITE_TEXT_METRICS metrics = {};

			auto const utf16 = to_utf16(label);
			
			IDWriteTextLayout* layout = nullptr;
			auto const hr = dwrite_->CreateTextLayout(
				utf16.c_str(),
				int32_t(utf16.size()),
				text_format.get(),
				0.0f,
				0.0f, 
				&layout);
			if (SUCCEEDED(hr)) 
			{
				layout->GetMetrics(&metrics);
				layout->Release();
			}

			return metrics;
		}

		void draw_tic(
			shared_ptr<ID2D1RenderTarget> const& ctx,
			shared_ptr<ID2D1Brush> const& brush,
			shared_ptr<IDWriteTextFormat> const& text_format,
			string const& label,
			D2D_VECTOR_2F const& center,
			float radius,
			float length,
			float rotation,
			float stroke)
		{
			auto const pt1 = D2D1::Point2F(center.x + radius, center.y);
			auto const pt2 = D2D1::Point2F(center.x + radius + length, center.y);

			auto xform = D2D1::Matrix3x2F::Rotation(
				rotation, D2D1::Point2F(center.x, center.y));
			ctx->SetTransform(xform);

			ctx->DrawLine(pt1, pt2, brush.get(), stroke);

			auto const utf16 = to_utf16(label);
						
			text_format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			text_format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			text_format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
			
			auto const metrics = measure(text_format, label);
			
			ctx->SetTransform(D2D1::Matrix3x2F::Identity());

			D2D1_RECT_F box;
			box.left = pt2.x + (metrics.height * 0.33f);
			box.top = pt2.y;

			{
				auto const r = (box.left - center.x);
				box.left = center.x + (float(cos(rotation * (PI / 180.0)) * r));
				box.top = center.y + (float(sin(rotation * (PI / 180.0)) * r));
			}

			box.right = box.left;
			box.bottom = box.top;
			
			ctx->DrawText(utf16.c_str(), 
				int32_t(utf16.size()), 
				text_format.get(), 
				box, 
				brush.get());
		}

		void draw_arc(
			shared_ptr<ID2D1RenderTarget> const& ctx, 
			shared_ptr<ID2D1Brush> const& brush,
			D2D_VECTOR_2F const& center,
			float radius,
			float sweep,
			float rotation,
			float stroke)
		{
			// draw arcs for highlights
			auto path = create_path();
			if (!path) {
				return;
			}
			
			ID2D1GeometrySink* sink = nullptr;
			if (FAILED(path->Open(&sink))) {
				return;
			}
			
			auto const radians = sweep * (PI / 180.0);
			sink->SetFillMode(D2D1_FILL_MODE_WINDING);

			auto pt = to_dips(ctx, center.x + radius, center.y);
			sink->BeginFigure(
				D2D1::Point2F(pt.x, pt.y),
				D2D1_FIGURE_BEGIN_FILLED
			);
			
			auto x = center.x + (float(cos(radians) * radius));
			auto y = center.y + (float(sin(radians) * radius));

			D2D1_ARC_SEGMENT arc;
			arc.arcSize = D2D1_ARC_SIZE_SMALL;
			arc.size.width = radius;
			arc.size.height = radius;
			arc.rotationAngle = 0.0f;
			arc.point = D2D1::Point2F(x, y);
			arc.sweepDirection = D2D1_SWEEP_DIRECTION_CLOCKWISE;

			sink->AddArc(&arc);
			sink->EndFigure(D2D1_FIGURE_END_OPEN);
			sink->Close();
			sink->Release();
			
			auto xform = D2D1::Matrix3x2F::Rotation(
				rotation, D2D1::Point2F(center.x, center.y));
			ctx->SetTransform(xform);

			ctx->DrawGeometry(path.get(), brush.get(), stroke);
		}
	};
}

shared_ptr<IAssets> create_assets()
{
	IWICImagingFactory* wic = nullptr;

	// initialize windows-imaging (WIC)
	auto hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wic));
	if (FAILED(hr)) {
		return nullptr;
	}

	// initialize Direct2D
	ID2D1Factory* d2d = nullptr;
	hr = D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED,
			&d2d);
	if (FAILED(hr)) {
		wic->Release();
		return nullptr;
	}

	// initialize DirectWrite for text
	IDWriteFactory* dwrite = nullptr;
	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&dwrite));
	if (FAILED(hr)) {
		wic->Release();
		d2d->Release();
		return nullptr;
	}

	return make_shared<Assets>(wic, d2d, dwrite);
}
