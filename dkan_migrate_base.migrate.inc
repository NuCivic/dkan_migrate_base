<?php

/**
 * @file
 * Migration file for dkan_migrate_base.
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
        'title' => t('CKAN Dataset Base'),
      ),
      'ckan_group_base' => array(
        'class_name' => 'MigrateCkanGroupBase',
        'group_name' => 'dkan',
        'title' => t('CKAN Group Base'),
      ),
      'ckan_resource_base' => array(
        'class_name' => 'MigrateCkanResourceBase',
        'group_name' => 'dkan',
        'title' => t('CKAN Resource Base'),
      ),
    ),
  );
  return $api;
}

class CKANListJSON extends MigrateListJSON {
  /**
   * Initialiazes.
   */
  public function __construct($list_url, $http_options = array()) {
    parent::__construct($list_url);
    $this->httpOptions = $http_options;
    $this->page = isset($http_options['page']) ? $http_options['page'] : '';
    $this->offset = isset($http_options['offset']) ? $http_options['offset'] : '';
  }
  /**
   * The default implementation assumes the IDs are top-level array elements.
   */
  protected function getIDsFromJSON(array $data) {
    $ids = array();
    $datasets = 0;
    $total = $this->page + $this->offset;
    foreach ($data['result'] as $item) {
      if ($datasets < $this->offset) {
        $datasets++;
        continue;
      }
      $ids[] = $item;
      $datasets++;
      if ($total && $datasets >= $total) {
        break;
      }
    }
    return $ids;
  }

  /**
   * Loads the JSON at the given URL, and returns an array of the IDs.
   *
   * @return array
   *   Array of IDs.
   */
  public function getIdList() {
    migrate_instrument_start("Retrieve $this->listUrl");

    if (substr($this->listUrl, 0, 6) == 'public') {
      $json = file_get_contents($this->listUrl);
    }
    else {
      $response = drupal_http_request($this->listUrl, $this->httpOptions);
      $json = $response->data;
    }
    migrate_instrument_stop("Retrieve $this->listUrl");
    if ($json) {
      $data = drupal_json_decode($json);
      if ($data) {
        return $this->getIDsFromJSON($data);
      }
    }
    Migration::displayMessage(t('Loading of !listurl failed:',
        array('!listurl' => $this->listUrl)));
    return NULL;
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
    // Only return page number if that many actually exist.
    if ($count > $this->page) {
      $count = $this->page;
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
    if (filter_var($item_url, FILTER_VALIDATE_URL)) {
      // Get the JSON object at the specified URL.
      $json = $this->loadJSONUrl($item_url);
      if ($json && isset($json->result)) {
        return $json->result;
      }
      if ($json && isset($json->error) && $json->error->message == 'Access denied') {
        $migration = Migration::currentMigration();
        $message = t('Access denied for !objecturl', array('!objecturl' => $item_url));
        $migration->getMap()->saveMessage(array($id), $message, MigrationBase::MESSAGE_ERROR);
        $result = $this->emptyItem($id);
        return $result;
      }
    }
    else {
      $json = json_decode(file_get_contents($item_url));
      return $json->result;
    }
    $migration = Migration::currentMigration();
    $message = t('Loading of !objecturl failed:', array('!objecturl' => $item_url));
    $migration->getMap()->saveMessage(array($id), $message, MigrationBase::MESSAGE_ERROR);
    return new stdClass();
  }

  /**
   * Creates a stub entry.
   */
  public function emptyItem($id) {
    $result = new stdClass();
    $result->id = substr($id, 0, 35);
    $result->title = t('Access denied for %id', array('%id' => $id));
    $result->name = $id;
    return $result;
  }
}

/**
 * Put our useful functions here.
 */
abstract class MigrateDKAN extends Migration {
  /**
   * Adds default dataset mappings.
   */
  public function addDefaultDatasetMappings() {
    $this->addFieldMapping('title', 'title');
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
  }

  /**
   * Prepare row for Datasets.
   */
  public function datasetPrepareRow(&$row) {
    $row->uid = $this->getUser($row->creator_user_id);
    $row->name = 'dataset/' . $row->name;
    $row->resource_ids = array();

    if (isset($row->resources)) {
      foreach ($row->resources as $resource) {
        $row->resource_ids[] = $this->getResourceId($resource->id);
      }
    }

    $tags = taxonomy_vocabulary_machine_name_load('tags');
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

    // Get unix timestamp values for dates.
    $row->metadata_created = strtotime($row->metadata_created);
    $row->metadata_modified = strtotime($row->metadata_modified);
    $row->spatialText = NULL;
    if (isset($row->extras)) {
      foreach ($row->extras as $extra) {
        if ($extra->key == 'spatial-text') {
          $row->spatialText = $extra->value;
        }
        elseif ($extra->key == 'spatial') {
          $row->spatial = $extra->value;
        }
        else {
          $row->field_additional_info_key[] = $extra->key;
          $row->field_additional_info_value[] = $extra->value;
        }
      }
    }
  }

  /**
   * Creates new Groups and adds it to the map.
   */
  public function createAndMapGroups(&$row) {
    if (isset($row->groups)) {
      foreach ($row->groups as $num => $group) {
        if ($group_nid = $this->getGroupId($group->id)) {
          $row->group_ids[] = $group_nid;
        }
        else {
          $group_node = $this->createGroupNode($group);
          // Adds the node to the map. This means that when the migration is
          // rolled back that the resources are removed as well. This adds the
          // newly created uuids and nids to the migrate_map_data_json table.
          $this->map->saveIDMapping($group_node, array($group_node->nid), 0, 0, '');
        }
      }
    }
  }

  /**
   * Creates new resources and adds it to the map.
   */
  public function createAndMapResources(&$row) {
    if (isset($row->resources)) {
      foreach ($row->resources as $num => $resource) {
        $resource_node = $this->createResourceNode($resource);
        // Adds the node to the map. This means that when the migration is
        // rolled back that the resources are removed as well. This adds the
        // newly created uuids and nids to the migrate_map_data_json table.
        $this->map->saveIDMapping($resource_node, array($resource_node->nid), 0, 0, '');
      }
    }
  }

  /**
   * Creates resource node.
   */
  public function createGroupNode($group) {
    // TODO: Check high water mark.

    global $user;
    $node = new stdClass();
    $node->is_new = TRUE;
    $node->language = 'und';
    $node->uid = $user->uid;
    $node->type = 'group';
    $node->title = $group->title;
    $node->uuid = $group->id;
    $node->body['und'][0]['safe_value'] = $group->description;
    if ($group->image_display_url) {
      $name = explode('/', $group->image_display_url->url);
      $name = $name[count($name) - 1];
      $uri = 'public://' . $name;
      $file = $this->downloadExternalFile($group->image_display_url, $uri);
      $node->field_image['und'][0] = $file;
    }
    node_save($node);
    return $node;
  }

  /**
   * Creates resource node.
   */
  public function createResourceNode($res) {
    // TODO: Check high water mark.
    // TODO: Check if UUID exits.

    // Creates a new resource for every linked file.
    // Linked files contain title, format, and accessURL.
    global $user;
    $node = new stdClass();
    $node->is_new = TRUE;
    $node->language = 'und';
    $node->uid = $user->uid;
    $node->type = 'resource';
    $node->title = $res->name;
    $node->uuid = $res->id;
    $node->body = $res->description;
    $res->format = strtolower($res->format);
    $node->changed = $res->created;
    if (isset($res->format)) {
      $format_vocab = taxonomy_vocabulary_machine_name_load('format');
      $this->createTax($res->format, 'format', $format_vocab->vid);
      $tax = taxonomy_get_term_by_name($res->format, 'format', $format_vocab->vid);
      $tid = array_keys($tax);
      if ($tid[0]) {
        $node->field_format['und'][0]['tid'] = $tid[0];
      }
    }
    $file = remote_stream_wrapper_file_load_by_uri($res->url);
    // Create a link to file for certain formats.
    if ($res->format == 'csv' || $res->format == 'zip') {
      if (!$file) {
        $file = remote_stream_wrapper_file_create_by_uri($res->url);
        $file->status = FALSE;
        file_save($file);
      }
      $node->field_link_remote_file['und'][0]['fid'] = $file->fid;
      $node->field_link_remote_file['und'][0]['display'] = 1;
    }
    else {
      // Link to API for all others.
      $node->field_link_api['und'][0]['url'] = $dist->accessURL;
    }
    node_save($node);
    return $node;
  }

  /**
   * Looks up nid for resource.
   */
  public function getResourceId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
  }

  /**
   * Looks up nid for group.
   */
  public function getGroupId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
  }

  /**
   * Prepare for Datasets.
   */
  public function datasetPrepare(&$node, &$row) {
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
          'uuid' => $creator_user_id,
        );
        // Get User name from CKAN API.
        $response = drupal_http_request($this->endpoint . 'user_show?id=' .  $creator_user_id);
        $json = $response->data;
        $data = drupal_json_decode($json);
        if ($name = $data['result']['name']) {
          $current_uid = db_query("SELECT uid from {users} WHERE name = :name", array(":name" => $name))->fetchField();
          if ($current_uid) {
            $account = user_load($current_uid);
            return $account->uid;
          }
          else {
            $new_user['name'] = $name;
            $account = user_save(NULL, $new_user);
            return $account->uid;
          }
        }
        else {
          return 1;
        }
      }
    }
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
      "tag_names" => "Tags",
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
      "group_ids" => "Group IDS",
    );
  }


  /**
   * Looks up format if exists, if not creates it.
   */
  public function createTax($name, $vocab_name, $vid) {
    if ($term = taxonomy_get_term_by_name($name, $vocab_name, $vid)) {
      $term = array_pop($term);
      return $term;
    }
    else {
      $new_term = new stdClass();
      $new_term->name = $name;
      $new_term->vid = $vid;
      $term = taxonomy_term_save($new_term);
      return $term;
    }
  }

  /**
   * Converts times with edge cases.
   */
  public function StringToTime($date) {
    if ($date = str_replace('-', '', $date)) {
      return strtotime($date);
    }
    else {
      return strtotime($date);
    }
  }

  /**
   * Gets Group Nid by title if exists.
   */
  public function getGroupNidByTitle($title) {
    $type = 'group';
    $result = db_query("SELECT n.nid FROM {node} n WHERE n.title = :title AND n.type = :type", array(":title"=> $title, ":type"=> $type));
    $nid = $result->fetchField();
    if ($nid) {
      return $nid;
    }
    else {
      return NULL;
    }
  }

  /**
   * Looks up nid for resource.
   */
  public function getResourceId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
    }
  }

  /**
   * Looks up nid for group.
   */
  public function getGroupId($uuid) {
    if ($nid = entity_get_id_by_uuid('node', array($uuid))) {
      return $nid[$uuid];
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
          'uuid' => $creator_user_id,
        );
        // Get User name from CKAN API.
        $response = drupal_http_request($this->endpoint . 'user_show?id=' .  $creator_user_id);
        $json = $response->data;
        $data = drupal_json_decode($json);
        if ($name = $data['result']['name']) {
          $current_uid = db_query("SELECT uid from {users} WHERE name = :name", array(":name" => $name))->fetchField();
          if ($current_uid) {
            $account = user_load($current_uid);
            return $account->uid;
          }
          else {
            $new_user['name'] = $name;
            $account = user_save(NULL, $new_user);
            return $account->uid;
          }
        }
        else {
          return 1;
        }
      }
    }
  }

  /**
   * Saves file locally.
   *
   * @param string $url
   *   Full url to file to download
   * @param string $uri
   *   Drupal uri of where to save file public://archive/test.pdf
   * @param int $save_mode
   *   File save mode from drupal core, ex FILE_EXISTS_REPLACE
   */
  public function downloadExternalFile($url, $uri, $save_mode = FILE_EXISTS_RENAME, $manage_file = TRUE) {

    $url_info = parse_url($url);
    $url_path_info = pathinfo($url_info['path']);

    // This helps with filenames with spaces.
    $url = $url_info['scheme'] . '://' . $url_info['host']  . $url_path_info['dirname'] . '/' . rawurlencode($url_path_info['basename']);

    // Need to remove the filename from the uri.
    $uri_target = file_uri_target($uri);
    $uri_scheme = file_uri_scheme($uri);
    $uri_path_info = pathinfo($uri_target);
    $directory = file_stream_wrapper_uri_normalize($uri_scheme . "://" . $uri_path_info['dirname']);

    if (file_prepare_directory($directory, FILE_CREATE_DIRECTORY)) {
      $drupal_result = drupal_http_request($url);
      if (!empty($drupal_result->data)) {
        $path = file_stream_wrapper_uri_normalize($uri);
        if ($manage_file) {
          $new_file = file_save_data($drupal_result->data, $path, $save_mode);
        }
        else {
          return file_unmanaged_save_data($drupal_result->data, $path, $save_mode);
        }
      }
      else {
        drupal_set_message("Error downloading file, no data recieved for " . $url);
        return FALSE;
      }
      $new_file->display = 1;
      return (array) $new_file;
    }
    else {
      drupal_set_message(t("Could not create directory"));
    }
  }

}

abstract class MigrateCkanBase extends MigrateDKAN {
  /**
   * Here we go.
   */
  public function __construct($arguments) {
    $this->endpoint = isset($arguments['endpoint']) ? $arguments['endpoint'] : 'http://demo.getdkan.com/api/3/action/';
    parent::__construct($arguments);
  }
}
