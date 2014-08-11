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
    'api' => 2,
    'groups' => array(
      'dkan' => array(
        'title' => t('DKAN'),
      ),
    ),
    'migrations' => array(
      'ckan' => array(
        'class_name' => 'MigrateDkanCkanMigration',
        'group_name' => 'dkan',
      ),
    ),
  );
  return $api;
}

/**
 * A dynamic migration that is reused for each source CSV file.
 */
class MigrateDkanCkanMigration extends Migration {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    parent::__construct($arguments);
    $fields = array(
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
    );
    $this->endpoint = 'http://demo.ckan.org/api/3/action/';
    $json_file = $this->endpoint . 'package_list';
    $item_url = $this->endpoint . 'package_showid=:id';
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

  }

  /**
   * Try and do this.
   */
  public function prepareRow($row) {
    // TODO:
    // slug
    // + needs to be stored as url alias
    // uuid
    // tags
    // + saved in prepareRow
    // resources
    // + separate migration
    // groups
    // + separate migration
    // Notes:
    // + we have author but not author email
    // + they don't have contact and contact email
    // + users won't be rolled back unless we create a new mapping

    // Look up user if they exist, if not create them.
    if (isset($row->creator_user_id) && $row->creator_user_id) {
      if ($uid = entity_get_id_by_uuid('user', array($row->creator_user_id))) {
        $row->uid = $uid[$row->creator_user_id];
      }
      else {
        $new_user = array(
          'uuid' => $row->creator_user_id,
        );
        $response = drupal_http_request($this->endpoint . 'user_show?id=' .  $row->creator_user_id);
        $json = $response->data;
        $data = drupal_json_decode($json);
        $new_user['name'] = $data['result']['name'];
        $account = user_save(NULL, $new_user);
        $row->uid = $account->uid;
      }
    }

    // Get unix timestamp values for dates.
    $row->metadata_created = strtotime($row->metadata_created);
    $row->metadata_modified = strtotime($row->metadata_modified);
  }
  public function prepare($node, stdClass $row) {
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
   *
   * @param mixed $id
   *
   * @return stdClass
   */
  public function getItem($id) {
    $item_url = $this->constructItemUrl($id);
    // Get the JSON object at the specified URL
    $json = $this->loadJSONUrl($item_url);
    if ($json) {
      return $json->result;
    }
    else {
      $migration = Migration::currentMigration();
      $message =  t('Loading of !objecturl failed:', array('!objecturl' => $item_url));
      $migration->getMap()->saveMessage(
        array($id), $message, MigrationBase::MESSAGE_ERROR);
      return NULL;
    }
  }
}
