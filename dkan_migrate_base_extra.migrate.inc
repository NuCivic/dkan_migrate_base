<?php

/**
 * @file
 * Migration file.
 */

/**
 * Implements hook_migrate_api().
 */
function dkan_migrate_base_extra_migrate_api() {
  $api = array(
    'api' => 2,
    'migrations' => array(
      'ckan_resource_base' => array(
        'class_name' => 'MigrateCkanResourceBase',
        'group_name' => 'dkan',
      ),
    ),
  );
  return $api;
}

/**
 * A dynamic migration that is reused for each source CSV file.
 */
class MigrateCkanResourceBase extends Migration {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $fields = array(
      "name" => "Name",
      "id" => "UUID",
      "description" => "Description",
      "format" => "Format",
      "created" => "Created Date",
      "last_modified" => "Modified Date",
      "url" => "URL",
      "revision_id" => "Revision ID",
      // Not sure if this is just Mimetype.
      "mimetype_inner" => "Mimetype Inner",
      // This doesn't actually exist but we are adding it later in prepareRow.
      "uid" => "User ID",
      "file" => "file",
    );

    $this->endpoint = 'http://demo.ckan.org/api/3/action/';

    // CKAN doesn't have a resource_list endpoint so this function creates one.
    $file_name = 'public://ckan-migrate-resource_list';
    dkan_migrate_base_create_resource_list($this->endpoint, $file_name);
    $json_file = file_create_url($file_name);

    $item_url = $this->endpoint . 'resource_show?id=:id';
    $this->source = new MigrateSourceList(new CKANListJSON($json_file),
          new CKANItemJSON($item_url, $fields), $fields);
    $this->map = new MigrateSQLMap(
            $this->machineName,
            array(
              'uuid' => array(
                'type' => 'varchar',
                'length' => 255,
                'not null' => TRUE,
                'description' => 'id',
              ),
            ),
            MigrateDestinationNode::getKeySchema()
        );

    $this->destination = new MigrateDestinationNode('resource');

    $this->addFieldMapping('title', 'name');
    $this->addFieldMapping('created', 'created');
    $this->addFieldMapping('body', 'description');
    $this->addFieldMapping('field_format', 'format');
    $this->addFieldMapping('field_upload', 'file');
    $this->addFieldMapping('field_link_remote_file', 'file_remote_link');
    $this->addFieldMapping('field_link_api', 'link');
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {
    // TODO:
    // + Find way to get user name for creator of resource
    // + Improve preview for files stuck as links.

    // Get unix timestamp values for dates.
    $row->created = strtotime($row->created);
    $row->last_modified = strtotime($row->last_modified);

    $row->name = $row->name ? $row->name : $row->description;
    // Tax terms in Drupal are case sensitive. It is better to have a single
    // 'html' term instead of 'html' and 'HTML'.
    // TODO: move to hook_node_update in dkan_dataset.
    $row->format = strtolower($row->format);
    $this->createFormat($row->format);

    // Decide which of DKAN's three fields is best for resource file.
    if ($row->url_type == 'upload') {
      $row->file = $row->url;
    }
    else {
      // CKAN API doesn't make it clear if file is link to API or just file.
      $field = field_info_instance('node', 'field_link_remote_file', 'resource');
      $extensions = explode(' ', $field['settings']['file_extensions']);
      if (in_array($row->format, $extensions) && $row->format != 'html') {
        $row->file_remote_link = $row->url;
      }
      else {
        $row->link = $row->url;
      }
    }
  }

  /**
   * Implements prepare.
   */
  public function prepare($node, stdClass $row) {
    // UUID doesn't show up as a field so had to do this.
    $node->uuid = $row->id;
  }

  /**
   * Looks up format if exists, if not creates it.
   */
  public function createFormat($name) {
    if (!taxonomy_get_term_by_name($name, 'format')) {
      $new_term = new stdClass();
      $new_term->name = $name;
      $new_term->vid = 1;
      taxonomy_term_save($new_term);
    }
  }
}
