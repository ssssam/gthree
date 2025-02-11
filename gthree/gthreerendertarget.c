#include <math.h>
#include <epoxy/gl.h>

#include "gthreerendertarget.h"
#include "gthreetexture.h"
#include "gthreeprivate.h"

typedef struct {
#ifdef DEBUG_LABELS
  int instance_id;
#endif

  int width;
  int height;

  graphene_rect_t scissor;
  gboolean scissor_test;

  graphene_rect_t viewport;

  gboolean depth_buffer;
  gboolean stencil_buffer;

  GthreeTexture *texture;
  GthreeTexture *depth_texture;

  guint gl_framebuffer;
  guint gl_depthbuffer;
} GthreeRenderTargetPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (GthreeRenderTarget, gthree_render_target, GTHREE_TYPE_RESOURCE)

static void
gthree_render_target_init (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);

#ifdef DEBUG_LABELS
  static int instance_count = 0;
  priv->instance_id = ++instance_count;
  g_autofree char *texture_name = g_strdup_printf ("rendertarget.%d.TEX", priv->instance_id);
#endif

  priv->scissor_test = FALSE;

  priv->texture = gthree_texture_new (NULL);

#ifdef DEBUG_LABELS
  gthree_texture_set_name (priv->texture, texture_name);
#endif

  gthree_texture_set_wrap_s (priv->texture, GTHREE_WRAPPING_CLAMP);
  gthree_texture_set_wrap_t (priv->texture, GTHREE_WRAPPING_CLAMP);

  gthree_texture_set_generate_mipmaps (priv->texture, FALSE);
  gthree_texture_set_mag_filter (priv->texture, GTHREE_FILTER_LINEAR);
  gthree_texture_set_min_filter (priv->texture, GTHREE_FILTER_LINEAR);

  gthree_texture_set_encoding (priv->texture, GTHREE_ENCODING_FORMAT_SRGB);
  gthree_texture_set_format (priv->texture, GTHREE_TEXTURE_FORMAT_RGBA);
  gthree_texture_set_data_type (priv->texture, GTHREE_DATA_TYPE_UNSIGNED_BYTE);
  gthree_texture_set_anisotropy (priv->texture, 1);

  priv->depth_buffer = TRUE;
  priv->stencil_buffer = TRUE;
}

static void
gthree_render_target_finalize (GObject *obj)
{
  GthreeRenderTarget *target = GTHREE_RENDER_TARGET (obj);
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);

  g_clear_object (&priv->texture);
  g_clear_object (&priv->depth_texture);

  G_OBJECT_CLASS (gthree_render_target_parent_class)->finalize (obj);
}


static void
gthree_render_target_set_used (GthreeResource *resource,
                               gboolean        used)
{
  GthreeRenderTarget *target = GTHREE_RENDER_TARGET (resource);
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);

  GTHREE_RESOURCE_CLASS (gthree_render_target_parent_class)->set_used (resource, used);

  if (priv->texture)
    gthree_resource_set_used (GTHREE_RESOURCE (priv->texture), used);

  if (priv->depth_texture)
    gthree_resource_set_used (GTHREE_RESOURCE (priv->depth_texture), used);
}

static void
gthree_render_target_unrealize (GthreeResource *resource)
{
  GthreeRenderTarget *target = GTHREE_RENDER_TARGET (resource);
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);

  if (priv->gl_framebuffer)
    {
      gthree_resource_lazy_delete (resource, GTHREE_RESOURCE_KIND_FRAMEBUFFER, priv->gl_framebuffer);
      priv->gl_framebuffer = 0;
    }

  if (priv->gl_depthbuffer)
    {
      gthree_resource_lazy_delete (resource, GTHREE_RESOURCE_KIND_RENDERBUFFER, priv->gl_depthbuffer);
      priv->gl_depthbuffer = 0;
    }
}

static void
gthree_render_target_class_init (GthreeRenderTargetClass *klass)
{
  GthreeResourceClass *resource_class = GTHREE_RESOURCE_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = gthree_render_target_finalize;
  resource_class->unrealize = gthree_render_target_unrealize;
  resource_class->set_used = gthree_render_target_set_used;
}

GthreeRenderTarget *
gthree_render_target_new (int width,
                          int height)
{
  GthreeRenderTarget *target = g_object_new (GTHREE_TYPE_RENDER_TARGET, NULL);
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);

  priv->width = width;
  priv->height = height;

  graphene_rect_init (&priv->scissor, 0, 0, width, height);
  graphene_rect_init (&priv->viewport, 0, 0, width, height);

  return target;
}

GthreeRenderTarget *
gthree_render_target_clone (GthreeRenderTarget *target)
{
  GthreeRenderTarget *clone;
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  GthreeRenderTargetPrivate *clone_priv;

  clone = gthree_render_target_new (priv->width, priv->height);
  clone_priv = gthree_render_target_get_instance_private (clone);

  clone_priv->scissor = priv->scissor;
  clone_priv->scissor_test = priv->scissor_test;

  clone_priv->viewport = priv->viewport;
  clone_priv->depth_buffer = priv->depth_buffer;
  clone_priv->stencil_buffer = priv->stencil_buffer;

  if (priv->depth_texture)
    clone_priv->depth_texture = g_object_ref (priv->depth_texture);

  gthree_texture_copy_settings (clone_priv->texture, priv->texture);

  return clone;
}

GthreeTexture *
gthree_render_target_get_texture (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->texture;
}

int
gthree_render_target_get_width (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->width;
}

int
gthree_render_target_get_height (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->height;
}

void
gthree_render_target_set_size (GthreeRenderTarget *target,
                               int width,
                               int height)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  priv->width = width;
  priv->height = height;

  graphene_rect_init (&priv->scissor, 0, 0, width, height);
  graphene_rect_init (&priv->viewport, 0, 0, width, height);
}

gboolean
gthree_render_target_get_depth_buffer (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->depth_buffer;
}

void
gthree_render_target_set_depth_buffer (GthreeRenderTarget *target,
                                       gboolean            depth_buffer)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  priv->depth_buffer = depth_buffer;
}

gboolean
gthree_render_target_get_stencil_buffer (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->stencil_buffer;
}

void
gthree_render_target_set_stencil_buffer (GthreeRenderTarget *target,
                                         gboolean            stencil_buffer)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  priv->stencil_buffer = stencil_buffer;
}

GthreeTexture *
gthree_render_target_get_depth_texture (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->depth_texture;
}

void
gthree_render_target_set_depth_texture (GthreeRenderTarget *target,
                                        GthreeTexture *texture)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);

  g_set_object (&priv->depth_texture, texture);
}

// Setup storage for internal depth/stencil buffers and bind to correct framebuffer
static void
setup_renderbuffer_storage (GthreeRenderTarget *render_target, guint gl_renderbuffer, gboolean is_multisample)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (render_target);

  glBindRenderbuffer (GL_RENDERBUFFER, gl_renderbuffer);
  if (priv->depth_buffer && ! priv->stencil_buffer )
    {
      if (is_multisample)
        {
#ifdef TODO
          var samples = getRenderTargetSamples( renderTarget );
          _gl.renderbufferStorageMultisample( _gl.RENDERBUFFER, samples, _gl.DEPTH_COMPONENT16, renderTarget.width, renderTarget.height );
#endif
        }
      else
        glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, priv->width, priv->height);
      glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gl_renderbuffer);
    }
  else if (priv->depth_buffer && priv->stencil_buffer )
    {
      if (is_multisample)
        {
#ifdef TODO
          var samples = getRenderTargetSamples( renderTarget );
          _gl.renderbufferStorageMultisample( _gl.RENDERBUFFER, samples, _gl.DEPTH_STENCIL, renderTarget.width, renderTarget.height );
#endif
        }
      else
        glRenderbufferStorage (GL_RENDERBUFFER, GL_DEPTH_STENCIL, priv->width, priv->height);

      glFramebufferRenderbuffer (GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, gl_renderbuffer);
    }
  else
    {
      guint gl_format = gthree_texture_format_to_gl (gthree_texture_get_format (priv->texture));
      guint gl_type = gthree_texture_data_type_to_gl (gthree_texture_get_data_type (priv->texture));
      guint gl_internal_format = gthree_texture_get_internal_gl_format (gl_format, gl_type);
      if (is_multisample)
        {
#ifdef TODO
          var samples = getRenderTargetSamples( renderTarget );
          _gl.renderbufferStorageMultisample( _gl.RENDERBUFFER, samples, glInternalFormat,
                                              renderTarget.width, renderTarget.height );
#endif
        }
      else
        glRenderbufferStorage (GL_RENDERBUFFER, gl_internal_format, priv->width, priv->height);
    }
  glBindRenderbuffer (GL_RENDERBUFFER, 0);
}

// Setup GL resources for a non-texture depth buffer
static void
setup_depth_renderbuffer (GthreeRenderTarget *render_target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (render_target);
  gboolean is_cube = FALSE;

#ifdef TODO
  is_cube = ( renderTarget.isWebGLRenderTargetCube === true );
#endif

  if (priv->depth_texture)
    {
      if (is_cube)
        {
          g_error ("target.depthTexture not supported in Cube render targets");
        }
#ifdef TODO
    setupDepthTexture( renderTargetProperties.__webglFramebuffer, renderTarget );
#endif
    }
  else
    {
      if (is_cube)
        {
#ifdef TODO
          renderTargetProperties.__webglDepthbuffer = [];
          for ( var i = 0; i < 6; i ++ )
            {
              _gl.bindFramebuffer( _gl.FRAMEBUFFER, renderTargetProperties.__webglFramebuffer[ i ] );
              renderTargetProperties.__webglDepthbuffer[ i ] = _gl.createRenderbuffer();
              setupRenderBufferStorage( renderTargetProperties.__webglDepthbuffer[ i ], renderTarget );
            }
#endif
        }
      else
        {
          glBindFramebuffer (GL_FRAMEBUFFER, priv->gl_framebuffer);
          glGenRenderbuffers (1, &priv->gl_depthbuffer);
#ifdef DEBUG_LABELS
          {
            g_autofree char *label = g_strdup_printf ("rendertarget.%d.RB.depth", priv->instance_id);
            glObjectLabel (GL_RENDERBUFFER, priv->gl_depthbuffer, strlen (label), label);
          }
#endif
          setup_renderbuffer_storage (render_target, priv->gl_depthbuffer, FALSE);
        }
    }

  glBindFramebuffer (GL_FRAMEBUFFER, 0);
}

static gboolean
is_power_of_two (guint value)
{
  return value != 0 && (value & (value - 1)) == 0;
}

gboolean
gthree_render_target_is_power_of_two (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return is_power_of_two (priv->width) && is_power_of_two (priv->height);
}

guint
gthree_render_target_get_gl_framebuffer (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return priv->gl_framebuffer;
}

const graphene_rect_t *
gthree_render_target_get_viewport (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  return &priv->viewport;
}

static gboolean
texture_needs_generate_mipmaps (GthreeTexture *texture, gboolean is_power_of_two)
{
  GthreeFilter min_filter = gthree_texture_get_min_filter (texture);

  return
    gthree_texture_get_generate_mipmaps (texture) &&
    is_power_of_two &&
    min_filter != GTHREE_FILTER_NEAREST &&
    min_filter != GTHREE_FILTER_LINEAR;
}

static void
generate_mipmap (guint target,
                 GthreeTexture *texture,
                 int width, int height)
{
  glGenerateMipmap (target);
  gthree_texture_set_max_mip_level (texture, log2 (MAX (width, height)));
}

void
gthree_render_target_update_mipmap (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  gboolean supports_mips = gthree_render_target_is_power_of_two (target);

  if (texture_needs_generate_mipmaps (priv->texture, supports_mips))
    {
      guint target = GL_TEXTURE_2D;
#ifdef TOOD
      if (renderTarget.isWebGLRenderTargetCube)
        target = GL_TEXTURE_CUBE_MAP;
#endif
      gthree_texture_bind (priv->texture, -1, target);
      generate_mipmap (target, priv->texture, priv->width, priv->height);
      glBindTexture (target, 0);
    }
}

void
gthree_render_target_realize (GthreeRenderTarget *target)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  gboolean is_cube = FALSE, is_multisample = FALSE, supports_mips = FALSE;
  GthreeTexture *texture;

  if (priv->gl_framebuffer)
    return;

  gthree_resource_set_realized_for (GTHREE_RESOURCE (target), gdk_gl_context_get_current ());
  glGenFramebuffers (1, &priv->gl_framebuffer);
#ifdef DEBUG_LABELS
  {
    g_autofree char *label = g_strdup_printf ("rendertarget.%d.FB", priv->instance_id);
    glObjectLabel (GL_FRAMEBUFFER, priv->gl_framebuffer, strlen (label), label);
  }
#endif

  texture = priv->texture;

#ifdef TODO
  is_cube = ( renderTarget.isWebGLRenderTargetCube === true );
  is_multisample = ( renderTarget.isWebGLMultisampleRenderTarget === true );
#endif
  supports_mips = gthree_render_target_is_power_of_two (target);

  // Setup framebuffer
  if (is_cube)
    {
#ifdef TODO
      renderTargetProperties.__webglFramebuffer = [];
      for ( var i = 0; i < 6; i ++ ) {
        renderTargetProperties.__webglFramebuffer[ i ] = _gl.createFramebuffer();
      }
#endif
    }
  else
    {
      if (is_multisample)
        {
#ifdef TODO
          renderTargetProperties.__webglMultisampledFramebuffer = _gl.createFramebuffer();
          renderTargetProperties.__webglColorRenderbuffer = _gl.createRenderbuffer();

          _gl.bindRenderbuffer( _gl.RENDERBUFFER, renderTargetProperties.__webglColorRenderbuffer );
          var glFormat = utils.convert( renderTarget.texture.format );
          var glType = utils.convert( renderTarget.texture.type );
          var glInternalFormat = getInternalFormat( glFormat, glType );
          var samples = getRenderTargetSamples( renderTarget );
          _gl.renderbufferStorageMultisample( _gl.RENDERBUFFER, samples, glInternalFormat, renderTarget.width, renderTarget.height );

          _gl.bindFramebuffer( _gl.FRAMEBUFFER, renderTargetProperties.__webglMultisampledFramebuffer );
          _gl.framebufferRenderbuffer( _gl.FRAMEBUFFER, _gl.COLOR_ATTACHMENT0, _gl.RENDERBUFFER, renderTargetProperties.__webglColorRenderbuffer );
          _gl.bindRenderbuffer( _gl.RENDERBUFFER, null );

          if ( renderTarget.depthBuffer )
            {
              renderTargetProperties.__webglDepthRenderbuffer = _gl.createRenderbuffer();
              setupRenderBufferStorage( renderTargetProperties.__webglDepthRenderbuffer, renderTarget, true );
            }
          _gl.bindFramebuffer( _gl.FRAMEBUFFER, null );
#endif
        }
    }

  // Setup color buffer
  if (is_cube)
    {
#ifdef TODO
      state.bindTexture( _gl.TEXTURE_CUBE_MAP, textureProperties.__webglTexture );
      setTextureParameters( _gl.TEXTURE_CUBE_MAP, renderTarget.texture, supportsMips );

      for ( var i = 0; i < 6; i ++ )
        {
          setupFrameBufferTexture( renderTargetProperties.__webglFramebuffer[ i ], renderTarget,
                                   _gl.COLOR_ATTACHMENT0, _gl.TEXTURE_CUBE_MAP_POSITIVE_X + i );
        }

      if ( textureNeedsGenerateMipmaps( renderTarget.texture, supportsMips ) )
        {
          generateMipmap( _gl.TEXTURE_CUBE_MAP, renderTarget.texture, renderTarget.width, renderTarget.height );
        }
      state.bindTexture( _gl.TEXTURE_CUBE_MAP, null );
#endif
    }
  else
    {
      gthree_texture_bind (texture, -1, GL_TEXTURE_2D);
      gthree_texture_set_parameters (GL_TEXTURE_2D, texture, supports_mips);
      gthree_texture_setup_framebuffer (texture,
                                        priv->width,
                                        priv->height,
                                        priv->gl_framebuffer,
                                        GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D);
      if (texture_needs_generate_mipmaps (texture, supports_mips))
        generate_mipmap (GL_TEXTURE_2D, texture, priv->width, priv->height);
      glBindTexture (GL_TEXTURE_2D, 0);
    }

  // Setup depth and stencil buffers
  if (priv->depth_buffer)
    setup_depth_renderbuffer (target);
}

void
gthree_render_target_download (GthreeRenderTarget *target,
                               guchar     *data,
                               gsize       stride)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  GdkRectangle all = { 0, 0, priv->width, priv->height };

  gthree_render_target_download_area (target, &all, data, stride);
}

void
gthree_render_target_download_area (GthreeRenderTarget *target,
                                    const GdkRectangle *area,
                                    guchar     *data,
                                    gsize       stride)
{
  GthreeRenderTargetPrivate *priv = gthree_render_target_get_instance_private (target);
  cairo_surface_t *surface;
  int alpha_size = 0;
  gboolean is_gles = gdk_gl_context_get_use_es (gdk_gl_context_get_current ());
  g_autofree guchar *row = g_malloc (stride);
  int i;

  if (is_gles)
    alpha_size = 1;
  else
    glGetTexLevelParameteriv (GL_TEXTURE_2D, 0, GL_TEXTURE_ALPHA_SIZE,  &alpha_size);

  surface = cairo_image_surface_create_for_data (data,
                                                 (alpha_size == 0) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_ARGB32,
                                                 area->width, area->height, stride);


  gthree_texture_bind (priv->texture, 0, GL_TEXTURE_2D);

  glFramebufferTexture2DEXT (GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
                             GL_TEXTURE_2D, gthree_texture_get_gl_texture (priv->texture), 0);
  glPixelStorei (GL_PACK_ALIGNMENT, 4);
  glPixelStorei (GL_PACK_ROW_LENGTH, cairo_image_surface_get_stride (surface) / 4);

  if (!is_gles)
    glReadPixels (area->x, area->y, area->width, area->height, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                  cairo_image_surface_get_data (surface));
  else
    glReadPixels (area->x, area->y, area->width, area->height, GL_RGBA, GL_UNSIGNED_BYTE,
                  cairo_image_surface_get_data (surface));

  /* y flip */
  for (i = 0; i < area->height / 2; i++)
    {
      guchar *top_row = cairo_image_surface_get_data (surface) + i * stride;
      guchar *bottom_row = cairo_image_surface_get_data (surface) + (area->height - 1 - i) * stride;
      memcpy (row, top_row, stride);
      memcpy (top_row, bottom_row, stride);
      memcpy (bottom_row, row, stride);
    }

  glPixelStorei (GL_PACK_ROW_LENGTH, 0);
  glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, 0);
}
