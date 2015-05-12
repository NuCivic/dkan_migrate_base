<?php

/**
 * @file
 * Migration Class for Datasets.
 */

/**
 * Migrate CKAN dataset.
 */
class MigrateCkanDatasetBase extends MigrateCkanBase {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $fields = $this->getCkanDatasetFields();

    $list_url = isset($arguments['list_url']) ? $arguments['list_url'] : 'package_list';
    $list_url = $this->endpoint . $list_url;
    $item_url = isset($arguments['item_url']) ? $arguments['item_url'] : 'package_show?id=:id';
    $item_url = $this->endpoint . $item_url;

    // Get the shared key for multi-threaded imports, or use the machine name.
    $this->shared_key = isset($arguments['shared_key']) ? $arguments['shared_key'] : $this->machineName;

    // The following are provided by the multi-threaded environment.
    $this->limit  = isset($arguments['limit']) ? $arguments['limit'] : '';
    $this->offset = isset($arguments['offset']) ? $arguments['offset'] : '';

    $this->highwaterField = array(
      'name' => 'revision_timestamp',
    );

    $this->source = new MigrateSourceList(new CKANListJSON(
      $list_url,
      array(
        'limit'  => $this->limit,
        'offset' => $this->offset,
      )
    ),
      new CKANItemJSON($item_url, $fields), $fields);

    $this->map = new MigrateSQLMap(
      $this->shared_key, // The shared key is provided by superclasses for compatibility with multi-threading, or is set to the machine name.
      array(
        'uuid' => array(
          'type'        => 'varchar',
          'length'      => 255,
          'not null'    => TRUE,
          'description' => 'id',
        ),
      ),
      MigrateDestinationNode::getKeySchema()
    );

    $this->destination = new MigrateDestinationNode('dataset', array('text_format' => 'html'));

    $this->addFieldMapping('field_license', 'license_title');
    $this->addFieldMapping('created', 'metadata_created');
    $this->addFieldMapping('changed', 'metadata_modified');
    $this->addFieldMapping('field_author', 'author');
    $this->addFieldMapping('field_contact_email', 'author_email');
    $this->addFieldMapping('uid', 'uid');
    $this->addFieldMapping('id', 'uuid');
    $this->addFieldMapping('path', 'name');
    $this->addFieldMapping('body', 'notes');
    $this->addFieldMapping('field_spatial_geographical_cover', 'spatialText');
    $this->addFieldMapping('field_spatial', 'spatial');
    $this->addFieldMapping('field_resources', 'resource_ids');
    $this->addFieldMapping('og_group_ref', 'group_ids');
    $this->addFieldMapping('field_tags', 'tag_names');
    $this->addFieldMapping('field_additional_info', 'field_additional_info_key');
    $this->addFieldMapping('field_additional_info:second', 'field_additional_info_value');
    $this->addFieldMapping('field_organization_ref', 'organization_id');

    // The title_255 property represents the dataset title, truncated to fit in 255 characters (a hard limit on the title field).
    $this->addFieldMapping('title', 'title_255');
  }

  /**
   * Creates list of fields for CKAN Dataset.
   */
  public function getCkanDatasetFields() {
    return array(
      "license_title"            => "License Title",
      "maintainer"               => "Maintainer",
      "relationships_as_object"  => "",
      "private"                  => "Private",
      "maintainer_email"         => "",
      "revision_timestamp"       => "Revision Date",
      "id"                       => "UUID",
      "metadata_created"         => "Created Date",
      "metadata_modified"        => "Modified Date",
      "author"                   => "Author",
      "author_email"             => "Author Email",
      "state"                    => "State",
      "version"                  => "Version",
      "creator_user_id"          => "Author UUID",
      "type"                     => "Node Type",
      "resources"                => "Resources",
      "num_resources"            => "Number of Resources",
      "tag_names"                => "Tags",
      "tracking_summary"         => "Tracking Summary",
      "groups"                   => "Groups",
      "license_id"               => "Licence ID",
      "relationships_as_subject" => "",
      "num_tags"                 => "Number of Tags",
      "organization"             => "Organization",
      "name"                     => "Name slug",
      "isopen"                   => "Is Open (bollean)",
      "url"                      => "URL",
      "notes"                    => "Description",
      "owner_org"                => "Owner Organization",
      "extras"                   => "Extras",
      "title"                    => "Title",
      "title_255"                => "Title (truncated to 255 characters)",
      "revision_id"              => "Revision ID",
      // This doesn't actually exist but we are adding it later in prepareRow.
      "uid"                      => "User ID",
      "resource_ids"             => "Resource IDS",
      "group_ids"                => "Group IDS",
      "organization_id"          => "Organization ID",
    );
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {

    // Adding instrumentation here as this call appears to take 7 secs.
    migrate_instrument_start("Get user [ name ::  $row->name ][ creator_user_id :: $row->creator_user_id ]");
    $row->uid = $this->getUser($row->creator_user_id);
    migrate_instrument_stop("Get user [ name :: $row->name ][ creator_user_id :: $row->creator_user_id ]");

    $row->name         = 'dataset/' . $row->name;
    $row->resource_ids = array();

    // Create a truncated version of the title that can fit Drupal fields max 255 characters.
    $row->title_255 = substr($row->title, 0, 255);

    if (isset($row->resources)) {
      foreach ($row->resources as $resource) {
        $row->resource_ids[] = $this->getResourceId($resource->id);
      }
    }

    // Initialize the tag names array.
    $row->tag_names = array();
    $tags           = taxonomy_vocabulary_machine_name_load('tags');
    if (isset($row->tags)) {
      foreach ($row->tags as $tag) {
        $this->createTax($tag->name, 'tags', $tags->vid);
        $row->tag_names[] = $tag->name;
      }
    }

    if (isset($row->groups)) {
      foreach ($row->groups as $group) {
        $row->group_ids[] = $this->getGroupId($group->id);
      }
    }

    if (isset($row->organization)) {
      $row->organization_id = $this->getOrganizationId($row->organization->id);
    }

    // Get unix timestamp values for dates.
    $row->metadata_created  = $this->StringToTime($row->metadata_created);
    $row->metadata_modified = $this->StringToTime($row->metadata_modified);
    $row->spatialText       = NULL;
    if (isset($row->extras)) {
      foreach ($row->extras as $extra) {
        if ($extra->key == 'spatial-text') {
          $row->spatialText = $extra->value;
        }
        elseif ($extra->key == 'spatial') {
          $row->spatial = $extra->value;
        }
        elseif (!empty($extra->value)) {
          $row->field_additional_info_key[]   = $extra->key;
          $row->field_additional_info_value[] = $extra->value;
        }
      }
    }
  }

  /**
   * Implements prepare.
   */
  public function prepare($node, stdClass $row) {
    $node->uuid = $row->id;
    // Turn off pathauto for import and set path.
    $node->path['pathauto'] = FALSE;
    $node->path['alias']    = $row->name;
    // Converts geojson spatial for saving field.
    if (isset($node->field_spatial[$node->language][0]['wkt']) && $spatial = $node->field_spatial[$node->language][0]['wkt']) {
      $geophp                                  = geophp_load();
      $geometry                                = geoPHP::load($spatial, 'json');
      $node->field_spatial[$node->language][0] = geofield_get_values_from_geometry($geometry);
    }
  }
}
