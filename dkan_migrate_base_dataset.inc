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

    $this->highwaterField = array(
      'name' => 'revision_timestamp',
    );

    $this->source = new MigrateSourceList(new CKANListJSON($list_url),
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

    $this->destination = new MigrateDestinationNode('dataset', array('text_format' => 'html'));

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
   * Implements prepareRow.
   */
  public function prepareRow($row) {
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
    $row->metadata_created = $this->StringToTime($row->metadata_created);
    $row->metadata_modified = $this->StringToTime($row->metadata_modified);
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
}
