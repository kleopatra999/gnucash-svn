

#include "config.h"
#include "gnc-druid-provider-desc-file.h"
#include "gnc-basic-gobject.h"

static void gnc_druid_provider_desc_file_class_init	(GNCDruidProviderDescFileClass *class);
static void gnc_druid_provider_desc_file_init		(GNCDruidProviderDescFile *gspaper);
static void gnc_druid_provider_desc_file_finalize	(GObject *obj);

static GNCDruidProviderDescClass *parent_class;

GNC_BASIC_GOBJECT(GNCDruidProviderDescFile, GNCDruidProviderDescFileClass,
		  G_TYPE_GNC_DRUID_PROVIDER_DESC,
		  gnc_druid_provider_desc_file_class_init,
		  gnc_druid_provider_desc_file_init,
		  gnc_druid_provider_desc_file_get_type,
		  gnc_druid_provider_desc_file_new)

static void
gnc_druid_provider_desc_file_class_init (GNCDruidProviderDescFileClass *klass)
{
  GObjectClass *object_class;
	
  object_class = G_OBJECT_CLASS (klass);
  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = gnc_druid_provider_desc_file_finalize;
}

static void
gnc_druid_provider_desc_file_finalize (GObject *obj)
{
  GNCDruidProviderDescFile *desc = (GNCDruidProviderDescFile *)obj;

  if (desc->text)
    g_free(desc->text);
  if (desc->last_directory)
    g_free(desc->last_directory);

  G_OBJECT_CLASS (parent_class)->finalize(obj);
}

static void
gnc_druid_provider_desc_file_init (GNCDruidProviderDescFile *o)
{
  o->parent.name = GNC_DRUID_PROVIDER_TYPE_FILE;
}

GNCDruidProviderDescFile*
gnc_druid_provider_desc_file_new_with_data(const gchar* title,
					   const gchar* text,
					   const gchar* last_dir,
					   GNCDruidProviderCB next_cb,
					   void (*remove_file)(gpointer, gpointer))
{
  GNCDruidProviderDescFile* desc;
  GNCDruidProviderDesc* gdp_desc;

  desc = gnc_druid_provider_desc_file_new();
  g_assert(desc);
  gdp_desc = GNC_DRUID_PROVIDER_DESC(desc);

  gdp_desc->next_cb = next_cb;
  desc->remove_file = remove_file;

  if (text)
    gnc_druid_provider_desc_file_set_text(desc, text);
  if (last_dir)
    gnc_druid_provider_desc_file_set_last_dir(desc, last_dir);
  if (title)
    gnc_druid_provider_desc_set_title(gdp_desc, title);

  return desc;
}

void
gnc_druid_provider_desc_file_set_text(GNCDruidProviderDescFile* desc,
				      const gchar* text)
{
  g_return_if_fail(desc);
  g_return_if_fail(IS_GNC_DRUID_PROVIDER_DESC_FILE(desc));
  g_return_if_fail(text);

  if (desc->text)
    g_free(desc->text);
  desc->text = g_strdup(text);
}

void
gnc_druid_provider_desc_file_set_last_dir(GNCDruidProviderDescFile* desc,
					  const gchar* last_dir)
{
  g_return_if_fail(desc);
  g_return_if_fail(IS_GNC_DRUID_PROVIDER_DESC_FILE(desc));
  g_return_if_fail(last_dir);

  if (desc->last_directory)
    g_free(desc->last_directory);
  desc->last_directory = g_strdup(last_dir);
}
