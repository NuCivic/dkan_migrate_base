<?php

/**
 * @file
 * Migration file.
 */

/**
 * Implements hook_migrate_api().
 */
function dkan_migrate_base_migrate_api() {
  $api = array(
    // Migrate API, not CKAN's of course.
    'api' => 2,
    'groups' => array(
      'dkan' => array(
        'title' => t('DKAN'),
      ),
    ),
    'migrations' => array(
      'ckan_dataset_base' => array(
        'class_name' => 'MigrateCkanDatasetBase',
        'group_name' => 'dkan',
        'title' => t('DKAN Dataset Base'),
      ),
    ),
  );
  return $api;
}

/**
 * A dynamic migration that is reused for each source CSV file.
 */
class MigrateCkanDatasetBase extends Migration {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $fields = $this->getCkanDatasetFields();
    $this->endpoint = 'http://demo.ckan.org/api/3/action/';
    $json_file = $this->endpoint . 'package_list';
    $item_url = $this->endpoint . 'package_show?id=:id';
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

    $this->destination = new MigrateDestinationNode('dataset');

    $this->addFieldMapping('title', 'title');
    $this->addFieldMapping('field_license', 'license_id');
    $this->addFieldMapping('created', 'metadata_created');
    $this->addFieldMapping('changed', 'metadata_modified');
    $this->addFieldMapping('field_author', 'author');
    $this->addFieldMapping('uid', 'uid');
    $this->addFieldMapping('id', 'uuid');
    $this->addFieldMapping('path', 'name');
    $this->addFieldMapping('body', 'notes');
    $this->addFieldMapping('field_spatial_geographical_cover', 'spatialText');
    $this->addFieldMapping('field_spatial', 'spatial');
    $this->addFieldMapping('field_resources', 'resource_ids');
  }

  /**
   * Creates list of fields for CKAN Dataset.
   */
  public function getCkanDatasetFields() {
    return array(
      "license_title" => "License Title",
      "maintainer" => "Maintaier",
      "relationships_as_object" => "",
      "private" => "Private",
      "maintainer_email" => "",
      "revision_timestamp" => "Revision Date",
      "id" => "UUID",
      "metadata_created" => "Created Date",
      "metadata_modified" => "Modified Date",
      "author" => "Author",
      "author_email" => "Author Email",
      "state" => "State",
      "version" => "Version",
      "creator_user_id" => "Author UUID",
      "type" => "Node Type",
      "resources" => "Resources",
      "num_resources" => "Number of Resources",
      "tags" => "Tags",
      "tracking_summary" => "Tracking Summary",
      "groups" => "Groups",
      "license_id" => "Licence ID",
      "relationships_as_subject" => "",
      "num_tags" => "Number of Tags",
      "organization" => "Organization",
      "name" => "Name slug",
      "isopen" => "Is Open (bollean)",
      "url" => "URL",
      "notes" => "Description",
      "owner_org" => "Owner Organization",
      "extras" => "Extras",
      "title" => "Title",
      "revision_id" => "Revision ID",
      // This doesn't actually exist but we are adding it later in prepareRow.
      "uid" => "User ID",
      "resource_ids" => "Resource IDS",
    );
  }

  /**
   * Implements prepareRow.
   */
  public function prepareRow($row) {
    // TODO:
    // tags
    // + saved in prepareRow
    // resources
    // + update user, add create
    // groups
    // + separate migration
    // related
    //
    // Notes:
    // + we have author but not author email
    // + the have maintainer_email we don't
    // + they don't have contact and contact email
    // + users won't be rolled back unless we create a new mapping
    // + we could add revisions
    // demo.ckan.org/api/3/action/package_revision_list?id=adur_district_spendin

    // Look up user if they exist, if not create them.
    $row->uid = $this->getUser($row->creator_user_id);
    $row->name = 'dataset/' . $row->name;

    foreach ($row->resources as $resource) {
      $row->resource_ids[] = $this->getResourceId($resource->id);
    }

    // Get unix timestamp values for dates.
    $row->metadata_created = strtotime($row->metadata_created);
    $row->metadata_modified = strtotime($row->metadata_modified);
    $row->spatialText = NULL;
    foreach ($row->extras as $extra) {
      if ($extra->key == 'spatial-text') {
        $row->spatialText = $extra->value;
      }
      elseif ($extra->key == 'spatial') {
        $row->spatial = $extra->value;
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
    $node->path['alias'] = $row->name;
    // Converts geojson spatial for saving field.
    if (isset($node->field_spatial[$node->language][0]['wkt']) && $spatial = $node->field_spatial[$node->language][0]['wkt']) {
      $geophp = geophp_load();
      $geometry = geoPHP::load($spatial, 'json');
      $node->field_spatial[$node->language][0] = geofield_get_values_from_geometry($geometry);
    }
  }

  /**
   * Looks up nid for resource.
   */
  public function getResourceId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
    else {
      // TODO: Create resource.
    }
  }

  /**
   * Looks up user if they exist, if not creates them.
   *
   * @param string $creator_user_id
   *   UUID for user from CKAN.
   *
   * @return string
   *   Drupal user_id (uid).
   */
  public function getUser($creator_user_id) {
    if (isset($creator_user_id) && $creator_user_id) {
      if ($uid = entity_get_id_by_uuid('user', array($creator_user_id))) {
        return $uid[$creator_user_id];
      }
      else {
        $new_user = array(
          'uuid' => $row->creator_user_id,
        );
        // Get User name from CKAN API.
        $response = drupal_http_request($this->endpoint . 'user_show?id=' .  $row->creator_user_id);
        $json = $response->data;
        $data = drupal_json_decode($json);
        $new_user['name'] = $data['result']['name'];
        $account = user_save(NULL, $new_user);
        return $account->uid;
      }
    }
  }
}

class CKANListJSON extends MigrateListJSON {
  /**
   * The default implementation assumes the IDs are top-level array elements.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    foreach ($data['result'] as $item) {
      $ids[] = $item;
    }
    return $ids;
  }

  /**
   * Implements computeCount().
   */
  public function computeCount() {
    $count = 0;
    if (empty($this->httpOptions)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        $count = count($data['result']);
      }
    }
    return $count;
  }
}

class CKANItemJSON extends MigrateItemJSON {

  /**
   * Parses for 'results' instead of base.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    foreach ($data['result'] as $item) {
      $ids[] = $item;
    }
    return $ids;
  }
  /**
   * Parses for 'results' instead of base.
   */
  public function computeCount() {
    $count = 0;
    if (empty($this->httpOptions)) {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        $count = count($data['result']);
      }
    }
    return $count;
  }

  /**
   * Implementors are expected to return an object representing a source item.
   */
  public function getItem($id) {
    $item_url = $this->constructItemUrl($id);
    // Get the JSON object at the specified URL.
    $json = $this->loadJSONUrl($item_url);
    if ($json) {
      return $json->result;
    }
    else {
      $migration = Migration::currentMigration();
      $message = t('Loading of !objecturl failed:', array('!objecturl' => $item_url));
      $migration->getMap()->saveMessage(
        array($id), $message, MigrationBase::MESSAGE_ERROR);
      return NULL;
    }
  }
}
