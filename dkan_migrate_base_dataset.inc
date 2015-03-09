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
    $this->page = isset($arguments['page']) ? $arguments['page'] : '';
    $this->offset = isset($arguments['offset']) ? $arguments['offset'] : '';

    $this->highwaterField = array(
      'name' => 'revision_timestamp',
    );

    $this->source = new MigrateSourceList(new CKANListJSON(
      $list_url,
      array('page' => $this->page,
            'offset' => $this->offset,
      )
    ),
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

    $this->addDefaultDatasetMappings();

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
    $this->datasetPrepareRow($row);
  }

  /**
   * Implements prepare.
   */
  public function prepare($node, stdClass $row) {
    // TODO: Move into datasetPrepare().
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
    $this->datasetPrepare($node, $row);
  }
}
